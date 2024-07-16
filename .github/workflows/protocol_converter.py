import sys
import re
import json

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    GRAY = '\033[37m'

def build_json(input_string):
    data = {}
    for line in get_lines(input_string):
        key, value = get_macro_key_value(line)
        if key:
            data[key] = value
    return data

def get_lines(input_string):
    line_buffer = ""

    # Handle escaped newlines
    input_string = re.sub(r'\\\n', '', input_string)


    for line in input_string.split('\n'):

        line_buffer += line
        line_buffer = line_buffer.rstrip("\n")
        pattern = re.compile(r'"\s*"')
        yield pattern.sub('', line_buffer)
        line_buffer = ""

def get_macro_key_value(line):
    # m = re.search('^#define\s+(?P<key>\w+)\s+"?(?P<value>[\w\.,%]+)"?', line);
    m = re.search(r'^#define\s+(?P<key>\w+)\s+"(?P<value>.*?)"\s*$', line);

    if m is None:
        m = re.search(r'^#define\s+(?P<key>\w+)\s+(?P<value>.*?)\s*$', line);
        if m is None:
          return (None, None)

    return (m.group('key'), m.group('value'));

def write_output(data, file_name):
    with open(file_name, 'w+') as fp:
        json.dump(data, fp, indent=4)

def create_class_database(input_string):

    database = {}
    debug = 1
    new_obj = {}
    index = 0

    for line in get_lines(input_string):
        regex_string = '^#define\s+GRID_CLASS_(?P<key>[0-9A-Z]*)_code\s+"?(?P<value>[0-9A-Za-z]*)"?'
        m = re.search(regex_string, line)

        if m != None:
            key = m.group('key')
            value = m.group('value')

            print("Current line: " + line)
            print("Matching with: " + regex_string)
            print(bcolors.OKGREEN + "Matched value: " + value + bcolors.ENDC)

            if key in database:
                print("OK")
            else:
                database[key] = {}
            database[key]["class_name"] = key
            database[key]["class_code"] = int(value, 16)
            database[key]["class_params"] = {}
            database[key]["class_params"]["INSTRUCTION"] = {}
            database[key]["class_params"]["INSTRUCTION"]["offset"] = 3
            database[key]["class_params"]["INSTRUCTION"]["length"] = 1

        regex_string = '^#define\s+GRID_CLASS_(?P<key>[0-9A-Z]*)_(?P<param>[0-9A-Z]*)_(?P<attr>\w+)\s+"?(?P<value>[0-9A-Za-z]*)"?'
        m = re.search(regex_string, line)

        if m != None:
            key = m.group('key')
            param = m.group('param')
            attr = m.group('attr')
            value = m.group('value')


            print("Current line: " + line)
            print("Matching with: " + regex_string)
            print(bcolors.OKGREEN + "Matched value: " + value + bcolors.ENDC)

            if param in database[key]["class_params"]:
                print("OK")
            else:
                database[key]["class_params"][param] = {}


            database[key]["class_params"][param][attr] = int(value)


    return database


def create_character_lookup(input_string):

    database = {}

    for line in get_lines(input_string):
        regex_string = '^#define\s+GRID_CONST_(?P<key>[0-9A-Z]*)\s+"?(?P<value>[0-9A-Za-z]*)"?'
        m = re.search(regex_string, line)

        if m != None:
            key = m.group('key')
            value = m.group('value')

            print("Current line: " + line)
            print("Matching with: " + regex_string)
            print(bcolors.OKGREEN + "Matched value: " + value + bcolors.ENDC)

            if key in database:
                print("OK")
            else:
                database[int(value, 16)] = key

    return database


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

def generate_lists_py(output_file_name, character_lookup, class_database):

    print(f"Creating file: '{output_file_name}' with definitions.")

    # Writing Python code to the output_file
    with open(output_file_name, 'w') as python_file:

        python_file.write("# Generated Python code from JSON\n\n")
        python_file.write("character_lookup = "+json.dumps(character_lookup, indent=2))

        python_file.write("\n# Generated Python code from JSON\n\n")
        python_file.write("class_database = "+json.dumps(class_database, indent=2))

def read_input_files_to_string(input_files):
    input = ""
    for input_file in input_files:
        with open(input_file, 'r') as file:
            input += file.read()
            input += '\n'
    return input

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Provide input file(s) name as first argument')

    input_files_comma_separated = sys.argv[1] if len(sys.argv) > 1 else 'grid_common/grid_protocol.h'
    output_file = sys.argv[2] if len(sys.argv) > 2 else 'out.json'
    constlist_file = sys.argv[3] if len(sys.argv) > 3 else 'lists.py'
    package_file = sys.argv[4] if len(sys.argv) > 4 else 'package.json'

    input_files = input_files_comma_separated.split(',')

    print('input:', input_files)
    print('output:', output_file)
    input_string = read_input_files_to_string(input_files)

    write_output(build_json(input_string), output_file)

    class_databse = create_class_database(input_string)
    # print(json.dumps(class_databse, indent=2))

    character_lookup = create_character_lookup(input_string)
    # print(json.dumps(character_lookup, indent=2))

    generate_lists_py(constlist_file, character_lookup, class_databse)

    if len(sys.argv)>4:
        generate_package_json(output_file, package_file)
