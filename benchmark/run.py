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

	def run(self, langs, times = 1):
		print(f'--- Running benchmark: {self.name} ---')
		for lang in langs:
			print(f'Running on lang: {colored(lang.name, lang.color)}')
			elapsed = 0.0
			for _ in range(times):
				elapsed += self.run_once(lang)
			print(f'Average time taken ({times} runs): ', colored(str(elapsed / times) + 's', 'green'))
			print('\n')

	def run_once(self, lang) -> float:
		start = datetime.now()
		lang.run(self.filename)
		end = datetime.now()
		return (end - start).total_seconds()

class Lang:
	r"""
	[exec_name] - string that is used as the executable to launch benchmark programs.
	Note that if the executable is not in the user's PATH then the exec_name must
	be the filepath of the executable relative to the directory from where this
	file is run. (eg - 'py' for Python and '..\bin\vy' for Vyse)
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
		os.system(f'{self.exec} {self.dir}//{filepath}{self.ext}')
		print('\n')

langs = [
	Lang('Vyse', '../build/vy', 'vyse', '.vy', 'yellow'),
	Lang('Lua', 'lua', 'lua', '.lua', 'magenta'),
	Lang('Python', 'python', 'python', '.py', 'blue')
]

benches = [
	BenchMark('Binary Trees', 'binary-trees'),
	BenchMark('Fibonacci Recursive', 'fib-recurs'),
	BenchMark('Fibonacci Iterative', 'fib'),
	BenchMark('For Loop', 'for'),
	BenchMark('Strings', 'string-equals'),
	BenchMark('Method Calls', 'method-call')
]

def run_all(langs, times):
	if not langs: 
		print("No such langauge.")
		return
	for bench in benches:
		bench.run(langs, times)

def find_bench(name):
	for b in benches:
		if b.filename == name: return b
	return None

parser = argparse.ArgumentParser()
parser.add_argument('--nocolor', help="Don't render any colors.", action='store_true')
parser.add_argument('--bench', help='Run a specific benchmark.')
parser.add_argument('--lang', help='only run the benchmark on a specific language.')
parser.add_argument('--times', help='number of times to run the benchmark')
args = parser.parse_args()

# Disable ANSI color codes
if args.nocolor: colored = lambda s, _ : str(s)

def get_langs():
	if args.lang:
		for lang in langs:
			if lang.name.lower() == args.lang.lower():
				return [lang]
		return None
	return langs

total_times = 1
if args.times:
	total_times = int(args.times) 
	if total_times < 0: total_times = 1

# Run a specific benchmark.
if args.bench:
	bench = find_bench(args.bench)
	if not bench:
		print(colored('Error', 'red'), f'No benchmark named {args.bench}')
	else:
		run_langs = get_langs()
		if not run_langs:
			print(f'No language called {args.lang}')
		else:
			bench.run(run_langs, total_times)
# Run all the benchmarks
else:
	run_all(get_langs(), total_times)
