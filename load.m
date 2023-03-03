function [results, info] = load(cap_files, varargin)
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
            [d_i, desc_i] = extract_raw_data(header.data_header, Bytes, header.row_size, exclude_incomplete_rows);
        case 'DIFF1'
            expanded_Bytes = expand_DIFF1(Bytes, header.row_size);
            [d_i, desc_i] = extract_raw_data(header.data_header, expanded_Bytes, header.row_size, exclude_incomplete_rows);
        otherwise
            error(['unexpected compression method: ' header.compression]);
    end
    
    % do some checks on the format
    datafn = fieldnames(d_i);
    assert(numel(datafn) == 1, 'expecting a single field at the highest level');
    datafn = datafn{1};
    
    % merge multiple files together
    if any(strcmpi(result_names,datafn))
        results.(datafn) = [results.(datafn) d_i.(datafn)]; % if this works
        info.(datafn).file = [info.(datafn).file {fn}]; % then we just need to record the file name
        if warn_on_multiple
            warning('data already contains an entry for "%s". Appending in the order encountered.', datafn);
        end
    else
        results.(datafn) = d_i.(datafn);
        info.(datafn).file = fn;
        info.(datafn).data_description = desc_i.data;
        info.(datafn).container_description = desc_i.container;
    end
    result_names{end+1} = datafn;
end

end

function [data, description] = extract_raw_data(header, Bytes, cols, exclude_incomplete_rows)
% given the header specification, extract data from the byte array
type_names = {'uint8',	'int8',	'uint16',	'int16',	'uint32',	'int32',	'uint64',	'int64',	'float32',	'float64',  'bool',	'char'};
type_sizes = [      1,       1,        2,         2,           4,         4,           8,         8,            4,          8,       1,      1];
basic_conv = @(type_name) @(bytes, count) reshape(typecast(reshape(bytes', [], 1), type_name), count, [])';
uint8_conv = basic_conv('uint8');
int8_conv = basic_conv('int8');
conv = {
    uint8_conv
    int8_conv
    basic_conv('uint16')
    basic_conv('int16')
    basic_conv('uint32')
    basic_conv('int32')
    basic_conv('uint64')
    basic_conv('int64')
    basic_conv('single')
    basic_conv('double')
    @(bytes, count) logical(uint8_conv(bytes,count))
    @(bytes, count) char(int8_conv(bytes,count))
    };

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


% then grab the data for each entry
offset = 0;
for i = 1:numel(header)
    if isempty(header(i).type)
        continue;
    end
    
    loc = strcmpi(type_names, header(i).type);
    assert(any(loc), 'unexpected type: "%s"', header(i).type);
    type_size = type_sizes(loc);
    converter = conv{loc};
    
    num_bytes = header(i).count*type_size;
    rng = (1:num_bytes) + offset;
    offset = offset + num_bytes;
    
    header(i).data = converter(Bytes(:,rng),header(i).count);
end

% finally reassemble the original data-structure
parents = {header.parent};
    function [data_result, data_desc] = resolve(parent)
        data_result = struct;
        child_loc = strcmp(parents, parent);
        children = header(child_loc);
        
        % special case for the top level
        if isempty(parent)
            n = children.name;
            [data_result.(n), data_desc.data.(n)] = resolve(n);
            data_desc.container.(n) = children.desc;
            return;
        end
        
        % otherwise recurse through the structure and generate the struct
        nc = numel(children);
        order = zeros(nc,1);
        for ii = 1:nc
            n = children(ii).name;
            if isempty(children(ii).type) % container
                [data_result.(n), data_desc.(n)] = resolve(n);
                data_desc.container.(n) = children(ii).desc;
            else
                data_result.(n) = children(ii).data; % data
                data_desc.(n) = children(ii).desc; % description
            end
            order(ii) = children(ii).ind; % recover the original order
        end
        [~,order_perm] = sort(order);
        data_result = orderfields(data_result,order_perm);
        data_desc = orderfields(data_desc,order_perm);
    end

[data, description] = resolve('');
end

function expanded_Bytes = expand_DIFF1(Bytes, cols)
error('TODO');
end