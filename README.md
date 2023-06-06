# Project 2: Command Line Shell

See this link for the entire spec: https://www.cs.usfca.edu/~mmalensek/cs326/assignments/project-2.html

Author: Colin Bindi

Project Information:

In this project, we implemented a shell of our own. A shell is the outermost layer of the operating system; examples include bash, csh, ksh, sh, tcsh, zsh. The shell I created prints its prompt and waits for user input. My shell is able to run commands in both the current directory and those in the PATH environment variable. This was accomplished by using execvp. My shell handles builtin commands such as "cd", "#", "history", "!!", "!" followed by a number or prefix, "jobs", and "exit". My shell also handles signal handling, I/O redirection, piping, and scripting mode.

To learn more about execvp use:

```bash
man execvp
```

How to compile edited files:

```bash
make
```

How to run shell:

```bash
./mash
```

Program Output:
```bash
$ ./mash
>>-[😌]-[1]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> echo 1
1
>>-[😌]-[2]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> echo 2
2
>>-[😌]-[3]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> echo 3
3
>>-[😌]-[4]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> history
1 echo 1
2 echo 2
3 echo 3
4 history
>>-[😌]-[5]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> ls
history.c  history.h  history.o	libshell.so  logger.h  Makefile  mash  README.md  shell.c  shell.o  tests  ui.c  ui.h  ui.o  util.c  util.h  util.o

>>-[😌]-[6]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> ls /usr
bin  include  lib  local  sbin  share  src

>>-[😌]-[7]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> echo hello there!
hello there!

>>-[🤯]-[8]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> ./blah
mash: no such file or directory: ./blah

>>-[🤯]-[9]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> cd /this/does/not/exist
chdir: no such file or directory: /this/does/not/exist

>>-[🙂]-[10]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> cd

>>-[😌]-[11]-[csbindi@csbindi-vm:~]-> whoami
mmalensek

>>-[😌]-[12]-[csbindi@csbindi-vm:~]-> cd P2-ColinBindi

# Create a directory with our full home directory in its path:
# **Must use the username outputted above from whoami)**
>>-[🙂]-[13]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> mkdir -p /tmp/home/csbindi/test

>>-[🙂]-[14]-[csbindi@csbindi-vm:~/P2-ColinBindi]-> cd /tmp/home/csbindi/test

# Note that the FULL path is shown here (no ~):
>>-[🙂]-[15]-[csbindi@csbindi-vm:/tmp/home/csbindi/test]-> pwd
/tmp/home/csbindi/test

# Create/overwrite 'my_file.txt' and redirect the output of echo there:
>>-[😌]-[16]-[csbindi@csbindi-vm:~]-> echo "hello world!" > my_file.txt
>>-[😌]-[17]-[csbindi@csbindi-vm:~]-> cat my_file.txt
hello world!

# Append text with '>>':
>>-[😌]-[18]-[csbindi@csbindi-vm:~]-> echo "hello world!" >> my_file.txt
>>-[😌]-[19]-[csbindi@csbindi-vm:~]-> cat my_file.txt
hello world!
hello world!

>>-[😌]-[20]-[csbindi@csbindi-vm:~]-> seq 100000 | wc -l
10000

>>-[😌]-[21]-[csbindi@csbindi-vm:~]-> cat /etc/passwd | sort > sorted_pwd.txt
(sorted contents written to 'sorted_pwd.txt')

# This is equivalent, but uses input redirection instead of cat:
>>-[😌]-[22]-[csbindi@csbindi-vm:~]-> sort < /etc/passwd > sorted_pwd.txt

# This is equivalent as well. Order of < and > don't matter:
>>-[😌]-[23]-[csbindi@csbindi-vm:~]-> sort > sorted_pwd.txt < /etc/passwd
```

Testing:

Run `make test` in order to run the test cases for the project.

```
$ make test
Running Tests: (v21)
 * 01 Command Execution    [1 pts]  [  OK  ]
 * 02 Scripting            [1 pts]  [  OK  ]
 * 03 Basic Builtins       [1 pts]  [  OK  ]
 * 04 Prompt               [2 pts]  [  OK  ]
 * 05 Mini History List    [1 pts]  [  OK  ]
 * 06 Small History List   [1 pts]  [  OK  ]
 * 07 Large History List   [1 pts]  [  OK  ]
 * 08 History !num         [1 pts]  [  OK  ]
 * 09 History !prefix      [1 pts]  [  OK  ]
 * 10 History !!           [1 pts]  [  OK  ]
 * 11 Basic Pipe           [1 pts]  [  OK  ]
 * 12 Large Data Pipe      [1 pts]  [  OK  ]
 * 13 Multi Pipe           [2 pts]  [  OK  ]
 * 14 IO Redirection       [1 pts]  [  OK  ]
 * 15 Pipes & Redirection  [1 pts]  [  OK  ]
 * 16 SIGINT handler       [1 pts]  [  OK  ]
 * 17 Background Jobs      [1 pts]  [  OK  ]
 * 18 Job List             [1 pts]  [  OK  ]
 * 19 History Navigation   [1 pts]  [  OK  ]
 * 20 Autocomplete         [1 pts]  [  OK  ]
 * 21 Documentation        [1 pts]  [  OK  ]
 * 22 Static Analysis      [1 pts]  [  OK  ]
 * 23 Basic Leak Check     [1 pts]  [  OK  ]
 * 24 Thorough Leak Check  [1 pts]  [  OK  ]
Execution complete. [26/26 pts] (100.0%)
 ____________
< Toot toot! >
 ------------
\                             .       .
 \                           / `.   .' "
  \                  .---.  <    > <    >  .---.
   \                 |    \  \ - ~ ~ - /  /    |
         _____          ..-~             ~-..-~
        |     |   \~~~\.'                    `./~~~/
       ---------   \__/                        \__/
      .'  O    \     /               /       \  "
     (_____,    `._.'               |         }  \/~~~/
      `----.          /       }     |        /    \__/
            `-.      |       /      |       /      `. ,~~|
                ~-.__|      /_ - ~ ^|      /- _      `..-'
                     |     /        |     /     ~-.     `-. _  _  _
                     |_____|        |_____|         ~ - . _ _ _ _ _>
```
