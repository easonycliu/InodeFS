# Basic File System

## Getting Started

```bash
$ chmod -R o+w InodeFS
$ docker pull shenjiahuan/cselab_env:1.0
$ sudo docker run -it --rm --privileged --cap-add=ALL -v `pwd`/InodeFS:/home/stu/InodeFS shenjiahuan/cselab_env:1.0 /bin/bash
  # now you will enter in a container environment, the codes you downloaded in cse-lab will apper in /home/stu/cse-lab in the container
$ cd InodeFS
$ make
```

If there's no error in make, an executable file part1_tester will be generated, and after you type:

```bash
$ ./part1_tester
```


## GRADING

```bash
$ sudo ./grade.sh
Passed A
Passed B
Passed C
Passed D
Passed E
Passed F -- Robustness
Part2 score: 100/100
```
