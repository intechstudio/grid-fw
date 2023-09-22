import sys
import re
import json

def build_json(file_name='grid_common/grid_protocol.h'):
    data = {};
    for line in get_lines(file_name):
        key, value = get_macro_key_value(line)
        if key:
            data[key] = value
    return data

def get_lines(file_name):
    with open(file_name) as fp:
        for line in fp:
            yield line

def get_macro_key_value(line):
    m = re.search('^#define\s+(?P<key>\w+)\s+"?(?P<value>[\w\.,%]+)"?', line);
    if m is None:
        return (None, None)
    return (m.group('key'), m.group('value'));

def write_output(data, file_name='out.json'):
    with open(file_name, 'w+') as fp:
        json.dump(data, fp, indent=4)

def convert(input_file_name, output_file_name):
    write_output(build_json(input_file_name), output_file_name)

def generate_package_json(output_file_name, package_file_name):

    with open(output_file_name, 'r') as file:
        data = json.load(file)

    major = data['GRID_PROTOCOL_VERSION_MAJOR']
    minor = data['GRID_PROTOCOL_VERSION_MINOR']
    patch = data['GRID_PROTOCOL_VERSION_PATCH']

    new_version = str(major)+"."+str(minor)+"."+str(patch)

    print(f"The key '{new_version}' was in the JSON file.")


    with open(package_file_name, 'r') as file:
        package_data = json.load(file)

    # Update the version field
    package_data['version'] = new_version

    # Open the package.json file for writing and update the version number
    with open(package_file_name, 'w') as file:
        json.dump(package_data, file, indent=2)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Provide input file name as first argument')
        sys.exit()
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else 'out.json'
    package_file = sys.argv[3] if len(sys.argv) > 3 else 'package.json'
    print('input:', input_file)
    print('output:', output_file)
    convert(input_file, output_file)
    generate_package_json(output_file, package_file)
