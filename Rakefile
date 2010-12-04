CXXFLAGS = "-Wall -g"
OPTIMIZE_FLAGS = "-O2"
BENCHMARK_INPUT_SIZE = 200 * 1024 * 1024

task :default => ['test', 'benchmark']

file 'HorspoolTest.o' => ['HorspoolTest.cpp', 'Horspool.cpp'] do
	sh "g++ #{CXXFLAGS} -c HorspoolTest.cpp -o HorspoolTest.o"
end

file 'StreamTest.o' => ['StreamTest.cpp', 'StreamBoyerMooreHorspool.cpp'] do
	sh "g++ #{CXXFLAGS} -c StreamTest.cpp -o StreamTest.o"
end

file 'TestMain.o' => 'TestMain.cpp' do
	sh "g++ #{CXXFLAGS} -c TestMain.cpp -o TestMain.o"
end

desc "Build test runner"
file 'test' => ['HorspoolTest.o', 'StreamTest.o', 'TestMain.o'] do
	sh "g++ #{CXXFLAGS} HorspoolTest.o StreamTest.o TestMain.o -o test"
end

desc "Build benchmark runner"
file 'benchmark' => ['benchmark.cpp', 'Horspool.cpp', 'StreamBoyerMooreHorspool.cpp'] do
	sh "g++ #{CXXFLAGS} #{OPTIMIZE_FLAGS} benchmark.cpp -o benchmark"
end

file 'benchmark_input/newlines.txt' do
	puts "Creating benchmark_input/newlines.txt"
	File.open('benchmark_input/newlines.txt', 'wb') do |f|
		newlines = "\n" * (1024)
		while f.pos < BENCHMARK_INPUT_SIZE
			f.write(newlines)
		end
	end
end

file 'benchmark_input/binary.dat' do
	sh "dd if=/dev/urandom of=benchmark_input/binary.dat bs=10240 count=#{BENCHMARK_INPUT_SIZE / 10240}"
end

file 'benchmark_input/alice-large.html' => 'benchmark_input/alice.html' do
	puts "Creating benchmark_input/alice-large.html"
	alice = File.open('benchmark_input/alice.html', 'rb') do |f|
		f.read
	end
	File.open('benchmark_input/alice-large.html', 'wb') do |f|
		while f.pos < BENCHMARK_INPUT_SIZE
			f.write(alice)
		end
	end
end

file 'benchmark_input/alice-small.html' => 'benchmark_input/alice.html' do
	puts "Creating benchmark_input/alice-small.html"
	alice = File.open('benchmark_input/alice.html', 'rb') do |f|
		f.read(1024 * 8)
	end
	File.open('benchmark_input/alice-small.html', 'wb') do |f|
		f.write(alice)
	end
end

def run_benchmark(haystack_title, haystack_file, needle = "I have control\n", iterations = 10)
	puts
	puts "# Matching #{needle.inspect} in \"#{haystack_title}\", #{iterations} iterations"
	result = system('./benchmark', haystack_file, needle, iterations.to_s)
	abort "*** Command failed" if !result
end

desc "Run benchmarks"
task :run_benchmark => ['benchmark', 'benchmark_input/newlines.txt', 'benchmark_input/binary.dat',
			'benchmark_input/alice-large.html', 'benchmark_input/alice-small.html'] do
	puts "######### Good needle #########"
	run_benchmark('Random binary data', 'benchmark_input/binary.dat')
	run_benchmark('Only newlines', 'benchmark_input/newlines.txt')
	run_benchmark('Alice in Wonderland (200 MB)', 'benchmark_input/alice-large.html')
	run_benchmark('Alice in Wonderland (8 KB)', 'benchmark_input/alice-small.html', "I have control\n", 500_000)
	
	puts
	puts "######### Bad needle, likely to hit worst-case performance #########"
	needle = "I have control\n\n"
	run_benchmark('Random binary data', 'benchmark_input/binary.dat', needle)
	run_benchmark('Only newlines', 'benchmark_input/newlines.txt', needle)
	run_benchmark('Alice in Wonderland (200 MB)', 'benchmark_input/alice-large.html', needle)
	run_benchmark('Alice in Wonderland (8 KB)', 'benchmark_input/alice-small.html', needle, 500_000)
end

desc "Clean compiled files"
task :clean do
	sh "rm -rf *.o *.dSYM"
	sh "rm -f benchmark test"
	sh "rm -f benchmark_input/alice-*.html"
	sh "rm -f benchmark_input/binary.dat"
	sh "rm -f benchmark_input/newlines.txt"
end
