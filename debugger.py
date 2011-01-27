# -*- coding: utf-8 -*-
"#!/usr/bin/env python"
import pdb
import os
import sys

def run(filename, lines):

    p = pdb.Pdb()

    for line in lines:
        err = p.set_break(filename, line)
        if err:
            print err
        else:
            bp = p.get_breaks(filename, line)[-1]
            print "Breakpoint %d at %s:%d" % (bp.number, bp.file, bp.line)

    p.run("from runpy import run_module; run_module('" + get_module(filename) + "', None, '__main__', True)")


def get_module(filename):

    return filename.rsplit("/")[-1][:-3]


def main():

    filename = sys.argv[1]
    lines = sys.argv[2]
    sys.path.insert(0, sys.argv[1].rsplit('/',1)[0])

    lines = [int(c) for c in lines.split(",") if c != '']

    run(filename, lines)


if __name__ == '__main__':
    main()
