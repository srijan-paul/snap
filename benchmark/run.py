import os
import sys
from datetime import datetime
from termcolor import colored
import argparse

# A single benchmark must have the same filename for all
# languages but can have different extensions. A benchmark
# object is defined as a [Name, Filename] pair where the name
# is the title of the benchmark and the  filename is it's name
# in the 'benchmark' directory.
class BenchMark:
	def __init__(self, name, filename):
		self.name = name
		self.filename = filename

	def run(self, langs):
		print(f'--- Running benchmark: {self.name} ---')
		for lang in langs:
			print(f'Running on lang: {colored(lang.name, lang.color)}')
			start = datetime.now()
			lang.run(self.filename)
			end = datetime.now()
			print(f'Time taken: ', colored((end - start).total_seconds(), 'green') + 's')
			print('\n')

class Lang:
	"""
	[exec_name] - string that is used as the executable to launch benchmark programs.
	Note that if the executable is not in the user's PATH then the exec_name must
	be the filepath of the executable relative to the directory from where this
	file is run. (eg - 'py' for Python and '..\\bin\\vy' for Vyse)
	[dir_name] name of the directory where all the benchmark programs for this
	language reside. (eg- 'lua' for the Lua language.)
	[ext] - file extension for the language (eg - '.lua' for Lua)
	"""
	def __init__(self, name, exec_name, dir_name, ext, color = 'magenta'):
		self.name = name
		self.exec = exec_name
		self.ext = ext
		self.dir = dir_name
		self.color = color

	def run(self, filepath):
		os.system(f'{self.exec} {self.dir}\\{filepath}{self.ext}')
		print('\n')

langs = [
	Lang('Vyse', '..\\bin\\vy', 'vyse', '.vy', 'yellow'),
	Lang('Lua', 'lua', 'lua', '.lua', 'magenta'),
	Lang('Python', 'py', 'python', '.py', 'blue')
]

benches = [
	BenchMark('Binary Trees', 'binary-trees'),
	BenchMark('Fibonacci', 'fib'),
	BenchMark('Method Calls', 'method-call')
]

def run_all():
	for bench in benches:
		bench.run(langs)

def find_bench(name):
	for b in benches:
		if b.filename == name: return b
	return None

parser = argparse.ArgumentParser()
parser.add_argument('--nocolor', help="Don't render any colors.", action='store_true')
parser.add_argument('--bench', help='Run a specific benchmark.')
args = parser.parse_args()

# Disable ANSI color codes
if args.nocolor: colored = lambda s, _ : str(s)

# Run a specific benchmark.
if args.bench:
	bench = find_bench(args.bench)
	if not bench:
		print(colored('Error', 'red'), f'No benchmark named {args.bench}')
	else:
		bench.run(langs)
# Run all the benchmarks
else:
	run_all()
