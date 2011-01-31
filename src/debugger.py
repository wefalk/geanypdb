# -*- coding: utf-8 -*-
"#!/usr/bin/env python"
import pdb
import os
import sys

def run(breaks, main_file):

    p = pdb.Pdb()

    for file, line in breaks:
        line = int(line)
        err = p.set_break(file, line)
        if err:
            print err
        else:
            bp = p.get_breaks(file, line)[-1]
            print "Breakpoint %d at %s:%d" % (bp.number, bp.file, bp.line)

    p.run("from runpy import run_module; run_module('" + get_module(main_file) + "', None, '__main__', True)")


def get_module(filename):

    return filename.rsplit("/")[-1][:-3]


def main():

    print sys.argv
    breaks = sys.argv[1]
    main_file = sys.argv[2]

    breaks = [tuple(c.split(":")) for c in breaks.split(",") if c != '']

    path = main_file.rsplit("/", 1)[0]
    sys.path.insert(0, path)

    run(breaks, main_file)


if __name__ == '__main__':
    main()
