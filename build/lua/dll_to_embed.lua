-- Base64 encoding implementation
local base64_chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

local function base64_encode(data)
    local result = {}
    
    for i = 1, #data, 3 do
        local b1, b2, b3 = data:byte(i, i+2)
        
        b1 = b1 or 0
        b2 = b2 or 0
        b3 = b3 or 0
        
        local n = b1 * 65536 + b2 * 256 + b3
        
        local c1 = math.floor(n / 262144) % 64
        local c2 = math.floor(n / 4096) % 64
        local c3 = math.floor(n / 64) % 64
        local c4 = n % 64
        
        result[#result + 1] = base64_chars:sub(c1 + 1, c1 + 1)
        result[#result + 1] = base64_chars:sub(c2 + 1, c2 + 1)
        
        -- Add padding if needed
        local remaining = #data - i + 1
        if remaining >= 2 then
            result[#result + 1] = base64_chars:sub(c3 + 1, c3 + 1)
        else
            result[#result + 1] = '='
        end
        
        if remaining >= 3 then
            result[#result + 1] = base64_chars:sub(c4 + 1, c4 + 1)
        else
            result[#result + 1] = '='
        end
    end
    
    return table.concat(result)
end

-- Read the DLL file
--local dll_path = '../example-dll/build/client.dll'
local dll_path = 'client.dll'
local dll_name = dll_path:match("([^/]+)$")
local dll_file = io.open(dll_path, 'rb')
if not dll_file then
    local h = io.popen('cmd /c for %I in ("' .. dll_path .. '") do @echo %~fI')
    local full_path = h and h:read('*l') or dll_path
    if h then h:close() end
    print('Error: Could not open DLL at: ' .. full_path)
    os.exit(1)
end

local dll_data = dll_file:read('*all')
dll_file:close()

print('Read ' .. #dll_data .. ' bytes from ' .. dll_name)

-- Encode to base64
print('Encoding to base64...')
local encoded = base64_encode(dll_data)

-- Generate embed.hpp
local embed_path = '../project/src/embed.hpp'
local embed_file = io.open(embed_path, 'w')
if not embed_file then
    local h = io.popen('cmd /c for %I in ("' .. embed_path .. '") do @echo %~fI')
    local full_path = h and h:read('*l') or embed_path
    if h then h:close() end
    print('Error: Could not create embed.hpp at: ' .. full_path)
    os.exit(1)
end

embed_file:write('#pragma once\n\n')
embed_file:write('// Auto-generated embedded DLL data\n')
embed_file:write('// Generated from ' .. dll_name .. '\n\n')

-- Split the base64 string into chunks of 1000 characters per line
local chunk_size = 1000
embed_file:write('static const char* EMBEDDED_DLL_BASE64 = \n')

for i = 1, #encoded, chunk_size do
    local chunk = encoded:sub(i, i + chunk_size - 1)
    embed_file:write('    "' .. chunk .. '"')
    if i + chunk_size <= #encoded then
        embed_file:write('\n')
    else
        embed_file:write(';\n')
    end
end

embed_file:write('\nstatic const unsigned int EMBEDDED_DLL_SIZE = ' .. #dll_data .. ';\n')
embed_file:close()

print('Successfully generated embed.hpp!')
print('Base64 encoded size: ' .. #encoded .. ' bytes')
print('Split into ' .. math.ceil(#encoded / chunk_size) .. ' lines')
