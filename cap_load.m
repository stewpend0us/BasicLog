function [results, info] = cap_load(cap_files, varargin)
% cap_files is a path to a cap file or the output of "dir" where each entry
% is a cap file
% optional inputs: exclude_incomplete_rows default true
%
% cap files start with a json header that describes the data followed by a
% single zero byte followed by the binary data

if ischar(cap_files)
    d.folder = '';
    d.name = cap_files;
elseif isstruct(cap_files)
    d = cap_files;
else
    error('expecting a file name or a struct (the result of a call to "dir")');
end

if isempty(d)
    warning('no files to load');
    return;
end

results = struct;
info = struct;
result_names = {};

exclude_incomplete_rows = pop(varargin, 'exclude_incomplete_rows', true);
warn_on_multiple = pop(varargin, 'warn_on_multiple', true);

% for each file
for i = 1:numel(d)
    
    % load the entire contents of the file
    fn = fullfile(d(i).folder,d(i).name);
    fid = fopen(fn, 'r');
    assert(fid ~= -1, ['unable to open ' fn]);
    [Bytes, count] = fread(fid,'uint8=>uint8');
    fclose(fid);
    
    if count == 0
        warning('File contains zero content: %s', fn);
        continue;
    end
    
    % extract the header
    fz = find(Bytes==0, 1); % the first zero is the end of the header string
    try
        header = jsondecode(char(Bytes(1:(fz-1)))');
    catch
        warning('failed to load: %s\n', fn);
        continue;
    end
    
    % separate the data from the header
    Bytes = Bytes((fz+1):end);
    
    % put the data in a struct format specified by the header
    switch header.compression
        case 'RAW'
            Bytes = fix_shape(Bytes, header.row_size, exclude_incomplete_rows);
            [data_i, description_i] = extract(header.data_header, Bytes);
        case 'DIFF1'
            expanded_Bytes = expand_DIFF1(Bytes, header.row_size, exclude_incomplete_rows);
            [data_i, description_i] = extract(header.data_header, expanded_Bytes);
        otherwise
            error(['unexpected compression method: ' header.compression]);
    end
    
    % do some checks on the format
    datafn = fieldnames(data_i);
    assert(numel(datafn) == 1, 'expecting a single field at the highest level');
    datafn = datafn{1};
    
    % merge multiple files together
    if any(strcmpi(result_names,datafn))
        results.(datafn) = [results.(datafn) data_i.(datafn)]; % if this works
        info.(datafn).file = [info.(datafn).file fn]; % then we just need to record the file name
        if warn_on_multiple
            warning('data already contains an entry for "%s". Appending in the order encountered.', datafn);
        end
    else
        results.(datafn) = data_i.(datafn);
        info.(datafn) = description_i;
        info.(datafn).file = {fn};
    end
    result_names{end+1} = datafn;
end

end

function Bytes = fix_shape(Bytes,cols,exclude_incomplete_rows)
% first get the Bytes in the correct shape
count = numel(Bytes);
orphaned = mod(count,cols);
if orphaned > 0
    if exclude_incomplete_rows % remove incompete rows
        Bytes = Bytes(1:(end-orphaned));
        count = count - orphaned;
    else % append zeros to incomplete rows (this is what fread would have done had we known the number of columns ahead of time)
        missing = cols - orphaned;
        Bytes = [Bytes;zeros(missing,1)];
        count = count + missing;
    end
end
Bytes = reshape(Bytes, cols, count/cols)';
end



function [data, description] = extract(header, Bytes)
% given the header specification, extract data from the byte array
% allowed data types
type_names = {'uint8',	'int8',	'uint16',	'int16',	'uint32',	'int32',	'uint64',	'int64',	'float32',	'float64',  'bool',	'char'};
matlab_names={'uint8',	'int8',	'uint16',	'int16',	'uint32',	'int32',	'uint64',	'int64',	 'single',	 'double',  'bool',	'char'};
type_sizes = [      1,       1,        2,         2,           4,         4,           8,         8,            4,          8,       1,      1];

% closure. returns a function that does the convertion for 'type_name'
    function c = basic_converter(type_name)
        function converted = conv(bytes, count)
            converted = typecast(reshape(bytes', [], 1), type_name);
            converted = reshape(converted, count, [])';
        end
        c = @conv;
    end

% cell array of converter functions for each type
converter = cellfun(@basic_converter, matlab_names(1:(end-2)), 'UniformOutput', false);
converter{end+1} = @(bytes, count) logical(converter{1}(bytes,count));
converter{end+1} = @(bytes, count) char(converter{2}(bytes,count));

    function info = extract_description(header)
        function children = find_children_of(parent_name)
            children = [];
            if isempty(parent_name)
                parent_qualified_name = {};
            else
                parent_qualified_name = strsplit(parent_name, '.');
            end
            for hi = 1:numel(header)
                if startsWith(header(hi).name, parent_name)
                    qualified_name = strsplit(header(hi).name, '.');
                    if numel(qualified_name) - numel(parent_qualified_name) == 1
                        children = [children header(hi)];
                    end
                end
            end
            [children.child] = deal([]);
            for hi = 1:numel(children)
                if ~any(strcmpi(type_names, children(hi).type))
                    children(hi).child = find_children_of(children(hi).name);
                end
            end
        end
        
        info = find_children_of('');
    end

    function data = extract_data(header, Bytes, data, index)
        n = numel(header);
        
        hi = 0;
        byte_offset = 0;
        while hi < n
            hi = hi + 1;
            qualified_name = strsplit(header(hi).name, '.');
            
            if isempty(header(hi).type)
                % it's a simple container (arrays not allowed)
                data = setfield(data, qualified_name{1:(end-1)}, {index}, qualified_name{end}, []);
                continue;
            end
            
            type_loc = strcmpi(type_names, header(hi).type);
            if any(type_loc)
                % it's a fundamental (arrays are contiguous)
                num_bytes = type_sizes(type_loc)*header(hi).count;
                rng = (1:num_bytes) + byte_offset;
                
                % add to the data struct
                data = setfield(data, qualified_name{1:(end-1)}, {index}, qualified_name{end}, converter{type_loc}(Bytes(:,rng),header(hi).count));
                byte_offset = byte_offset + num_bytes;
                continue;
            end
            
            % it's a structure (we can have messy arrays of these...)
            size = str2double(header(hi).type);
            assert(~isnan(size),'unexpected type: "%s"', header(hi).type);
%             header(hi).type = size;
            
            % the following headers are 'guaranteed' to be children of
            % this header. package them all up.
            pi = hi;
            parent_name = header(pi).name;
            all_children = [];
            while hi < n
                hi = hi + 1;
                if startsWith(header(hi).name, parent_name)
                    all_children = [all_children header(hi)];
                else
                    % hi points to first non-child entry
                    % aka the next header in the list
                    break; 
                end
            end
            
            % for each element in the structure array
            % recurse through the list of children and expand each one out
            for i = 1:header(pi).count
                rng = (1:size) + byte_offset;
                data = extract_data(all_children, Bytes(:,rng), data, i);
                byte_offset = byte_offset + size;
            end
        end
        
        
    end

    function [data, description] = order_children(data, description)
        fn = fieldnames(data);
        n = numel(fn);
        assert(n == numel(description), 'mismatch');
        for i = 1:n
            if isempty(description(i).child)
                continue;
            end
            order = [description(i).child.ind];
            description(i).child = rmfield(description(i).child,'ind');
            [~, order_perm] = sort(order);
            data.(fn{i}) = orderfields(data.(fn{i}), order_perm);
            description(i).child = description(i).child(order_perm);
            [data.(fn{i}), description(i).child] = order_children(data.(fn{i}), description(i).child);
        end
    end

data = extract_data(header,Bytes,struct,1);
description = extract_description(header);
[data, description] = order_children(data, description);
description = rmfield(description,'ind');

end

function expanded_Bytes = expand_DIFF1(Bytes, cols, exclude_incomplete_rows)
num_bits = floor(cols/8) + double(mod(cols,8) > 0);
expanded_Bytes = uint8([]);


derp = true;
while numel(Bytes) >= num_bits
    prefix = Bytes(1:num_bits);
    Bytes(1:num_bits) = [];
    
    exp_prefix = [];
    for i = 1:num_bits
        exp_prefix = [exp_prefix bitget(prefix(i),1:8)];
    end
    exp_prefix = logical(exp_prefix(1:cols));
    count = sum(exp_prefix);
    if numel(Bytes) < count
        if exclude_incomplete_rows
            break; % already done
        else
            Bytes = [Bytes; zeros(count - numel(Bytes),1)]; % pad with zeros
        end
    end
    
    row = zeros(1,cols);
    row(exp_prefix) = Bytes(1:count);
    expanded_Bytes = [expanded_Bytes; row];
    Bytes(1:count) = [];
end
end