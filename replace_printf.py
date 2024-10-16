import os
import re

def process_file(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    new_lines = []
    inside_debug = False

    for line in lines:
        stripped_line = line.strip()
        # Check for entering a DEBUG block
        if re.match(r'^\s*#\s*ifdef\s+DEBUG', stripped_line):
            inside_debug = True
            new_lines.append(line)
            continue
        # Check for exiting a DEBUG block
        elif re.match(r'^\s*#\s*endif', stripped_line):
            inside_debug = False
            new_lines.append(line)
            continue

        if inside_debug:
            # Replace printf with append_to_log inside DEBUG blocks
            new_line = re.sub(r'\bPrintf\b', 'append_to_log', line)
            new_lines.append(new_line)
        else:
            new_lines.append(line)

    # Write the changes back to the file
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

def process_directory(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.c') or file.endswith('.h'):
                file_path = os.path.join(root, file)
                print(f'Processing {file_path}')
                process_file(file_path)

if __name__ == '__main__':
    # Replace 'your_source_directory' with the path to your source code directory
    source_directory = 'T:\Temp\Amiga\Local Disk\MyProjects\iTidy'
    process_directory(source_directory)
