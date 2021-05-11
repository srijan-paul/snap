import os
from datetime import datetime
from termcolor import colored

class BenchMark:
	def __init__(self, name, filename):
		self.name = name
		self.filename = filename

	def run(self, langs):
		print(f'--- {self.name} ---')
		for lang in langs:
			print(f'Running on lang: {lang.name}')
			start = datetime.now()
			lang.run(self.filename)
			end = datetime.now()
			print(f'Time taken: ', colored((end - start).total_seconds(), 'green'))
			print('\n')

class Lang:
	def __init__(self, name, exec_name, dir_name, ext):
		self.name = name
		self.exec = exec_name
		self.ext = ext
		self.dir = dir_name

	def run(self, filepath):
		os.system(f'{self.exec} {self.dir}\\{filepath}{self.ext}')
		print('\n')

langs = [
	Lang('Vyse', '..\\bin\\vy', 'vyse', '.vy'),
	Lang('Lua', 'lua', 'lua', '.lua'),
	Lang('Python', 'py', 'python', '.py')
]

benches = [
	BenchMark('Binary Trees', 'binary-trees'),
	BenchMark('Fibonacci', 'fib'),
	BenchMark('Method Calls', 'method-call')
]

for bench in benches:
	bench.run(langs)