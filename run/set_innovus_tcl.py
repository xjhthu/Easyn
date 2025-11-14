import argparse

def replace_design_placeholder(input_file, output_file, replacement_string):
    with open(input_file, 'r') as f:
        content = f.read()

    modified_content = content.replace('${design_name}', replacement_string)
    modified_content = modified_content.replace('$design_name', replacement_string)

    with open(output_file, 'w') as f:
        f.write(modified_content)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Replace placeholder in Tcl file")
    parser.add_argument("input_file", help="Path to input Tcl file")
    parser.add_argument("output_file", help="Path to output Tcl file")
    parser.add_argument("replacement_string", help="String to replace the placeholder")
    
    args = parser.parse_args()

    input_file_path = args.input_file
    output_file_path = args.output_file
    new_design_string = args.replacement_string

    replace_design_placeholder(input_file_path, output_file_path, new_design_string)
