METHOD = depth
#FILE = test_file_size_2.txt
FILE = test_file_size_5.txt
#FILE = test_file_size_8.txt
OUTPUT = output.txt

all:
	gcc -o project_freecell project_freecell.c
	./project_freecell $(METHOD) $(FILE) $(OUTPUT)

clean:
	rm -f project_freecell output.txt
