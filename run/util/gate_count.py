import argparse
import json

parser = argparse.ArgumentParser(description='Count the number of gates.')
parser.add_argument('--file', type=str, help='Input JSON file path')
args = parser.parse_args()

file_path = args.file

with open(file_path, 'r') as file:
    data = json.load(file)

gate_count = 0
for key, value in data.items():
    if isinstance(value, dict) and value.get('isPlaced', False):
        gate_count += 1

print(f'Total number of gates: {gate_count}')