CXXFLAGS = "-Wall -g"

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

file 'test' => ['HorspoolTest.o', 'StreamTest.o', 'TestMain.o'] do
	sh "g++ #{CXXFLAGS} HorspoolTest.o StreamTest.o TestMain.o -o test"
end

file 'benchmark' => ['benchmark.cpp', 'Horspool.cpp', 'StreamBoyerMooreHorspool.cpp'] do
	sh "g++ #{CXXFLAGS} -O2 benchmark.cpp -o benchmark"
end

def run_benchmark(haystack_title, haystack_file, needle = "I have control\n", iterations = 10)
	puts
	puts "# Matching #{needle.inspect} in \"#{haystack_title}\", #{iterations} iterations"
	result = system('./benchmark', haystack_file, needle, iterations.to_s)
	abort "*** Command failed" if !result
end

task :run_benchmark => 'benchmark' do
	puts "######### Good needle #########"
	run_benchmark('Random binary data', 'binary.dat')
	run_benchmark('Only newlines', 'newlines.txt')
	run_benchmark('Alice in Wonderland (200 MB)', 'alice.html')
	run_benchmark('Alice in Wonderland (8 KB)', 'alice-small.html', "I have control\n", 500_000)
	
	puts
	puts "######### Bad needle, likely to hit worst-case performance #########"
	needle = "I have control\n\n"
	run_benchmark('Random binary data', 'binary.dat', needle)
	run_benchmark('Only newlines', 'newlines.txt', needle)
	run_benchmark('Alice in Wonderland (200 MB)', 'alice.html', needle)
	run_benchmark('Alice in Wonderland (8 KB)', 'alice-small.html', needle, 500_000)
end