#!/usr/bin/env python3

###############################################################################/
# Copyright (c) 2022 Nerian Vision GmbH
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
###############################################################################/

#
# This helper script auto-generates pydoc comments (Google-style syntax)
#  from the Doxygen comments in the specified Nerian headers.
#

import sys
import os
import re

def print_error(what):
    sys.stderr.write(what + '\n')

class RegexMatcher(object):
    def __init__(self):
        self.result = None
    def search(self, regex, where):
        self.result = re.search(regex, where)
        return self.result is not None
    def group(self, i=0):
        return self.result.group(i)
    def groups(self):
        return self.result.groups()

class DocstringExtractor(object):
    def __init__(self):
        self.docstrings = {}
        pass

    def snake_case(self, fnname):
        '''Convert mixed case to Python methods' snake case'''
        fnname_snake = ''
        for c in fnname:
            if c.isupper():
                fnname_snake += '_' + c.lower()
            else:
                fnname_snake += c
        # Some conventional exceptions :)
        fnname_snake = fnname_snake.replace('r_o_i', 'roi')
        return fnname_snake

    def beautified_docstring(self, comment, indent=8):
        ds = ''
        cs = [l.strip() for l in comment.split('\n')] # if l.strip()!='']
        # remove leading blank lines
        reallines = list(filter(lambda x: x>0, [c!='' for c in cs]))
        if len(reallines):
            cs = cs[reallines[0]:]
        #
        printed_kwarg = False
        extra_indent = 0
        for i, c in enumerate(cs):
            if c.strip() == '':
                extra_indent = 0
            next_is_param = False
            cnew = ''
            increase_extra_indent = 0
            for j, w in enumerate(c.split()):
                if w in ['\\brief', '\\c']:
                    pass
                elif w in ['\\return']:
                    ds += '\n'
                    ds += ' '*indent + 'Returns:\n'
                    extra_indent = 4
                    increase_extra_indent = 4
                elif w in ['\\param']:
                    if not printed_kwarg:
                        ds += ' '*indent + 'Args:\n'
                    extra_indent = 4
                    increase_extra_indent = 4
                    printed_kwarg = True
                    next_is_param = True
                    pass
                elif w.startswith('\\'):
                    cnew += (' ' if len(cnew) else '') + w[1].upper()+w[2:]+': '
                else:
                    cnew += (' ' if len(cnew) else '') + w
                    if next_is_param:
                        cnew += ':'
                        next_is_param = False
            ds += ' '*indent + ' '*extra_indent + ("'''" if i==0 else "") + cnew + ("'''\n" if i==len(cs)-1 else "")
            ds += '\n'
            extra_indent += increase_extra_indent
        return ds

    def generate(self, basedir, filename):
        with open(basedir + '/' + filename, 'r') as f:
            in_comment = False
            comment = ''
            names = []
            currentname = ''
            currentargs = ''
            level = 0
            restl =''
            for rawl in [ll.strip() for ll in f.readlines()]:
                l = restl + rawl
                had_restl = len(restl) > 0
                restl = ''
                apply_comment = False
                if in_comment:
                    end = l.find('*/')
                    thisline = (l if end<0 else l[:end]).lstrip('*').strip()
                    #if thisline != '':
                    comment += '\n' + thisline
                    if end >= 0:
                        in_comment = False
                else:
                    start = l.find('/**')
                    if start >= 0:
                        currentname = '' # force finding new name
                        currentargs = ''
                        in_comment = True
                        comment = l[start+3:]
                    else:
                        rem = RegexMatcher()
                        if rem.search(r'(namespace|class|enum)([^:]*).*[{;]', l):
                            if comment != '':
                                cls = rem.group(2).strip().split()[-1]
                                currentname = cls
                                currentargs = ''
                                apply_comment = True
                        elif rem.search(r'[ \t]*(.*)\(', l):   # match word and opening paren
                            if currentname == '':
                                cls = rem.group(1).strip().split()[-1]
                                currentname = cls
                            if rem.search(r'[ \t]*([^(]*)\((.*)\).*[{;]', l) and l.count('(') == l.count(')'):    #:  # match function
                                if l.count('(') == l.count(')'):
                                    # reduce argument list (just names, no types or defaults)
                                    args_just_names = [(a.split('=')[0].strip().split()[-1] if a.strip()!='' else '') for a in rem.group(2).split(',')]
                                    currentargs = '(' + (', '.join(args_just_names)) + ')'
                                if comment != '':
                                    apply_comment = True
                            else: # match partial fn or something like it
                                restl = l   # save line for next iteration
                                continue    # and proceed to next line
                        else:
                            pass
                if apply_comment:
                    ns = names + [currentname+currentargs]
                    ns = [n for n in ns if n!='']
                    name = '::'.join(ns)
                    if name in self.docstrings and len(ns)>1: # warn, but not for the namespace doc
                        print_error('Note: not overwriting previous docstring for '+name)
                    else:
                        self.docstrings[name] = self.beautified_docstring(comment, indent=8)

                    comment = ''
                for j in range(l.count('{')):
                    level += 1
                    names.append(currentname+currentargs)
                    currentname = ''
                    currentargs = ''
                for j in range(l.count('}')):
                    level -= 1
                    names = names[:-1]
                    currentname = ''
                    currentargs = ''

    def store_docstrings_to_file(self, filename='', fobj=None):
        f = open(filename, 'w') if fobj is None else fobj
        f.write('''
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !!  CAUTION                                                        !!
# !!                                                                 !!
# !!  This file is autogenerated from the libvisiontransfer headers  !!
# !!  using autogen_docstrings.py - manual changes are not permanent !!
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!''' + '\n\n')
        f.write('_NERIAN_COMPILED_DOCSTRINGS = {\n')
        for name, comment in self.docstrings.items():
            f.write("    '"+ name + "': \\\n")
            f.write(comment.rstrip('\n') + ',\n')
        f.write('}\n')
        f.write("\n# Also add parameter-less versions for convenience (duplicates overwritten)\n")
        f.write("for __k in list(_NERIAN_COMPILED_DOCSTRINGS):\n")
        f.write("    if __k.count('('):\n")
        f.write("        _NERIAN_COMPILED_DOCSTRINGS[__k.split('(')[0]] = _NERIAN_COMPILED_DOCSTRINGS[__k]\n\n")
        if fobj is not None:
            f.close()

if __name__=='__main__':
    basedir = os.getenv("LIBVISIONTRANSFER_SRCDIR", '../..')
    if os.path.isdir(basedir):
        d = DocstringExtractor()
        for filename in [
            'visiontransfer/deviceparameters.h',
            'visiontransfer/imageset.h',
            'visiontransfer/imageprotocol.h',
            'visiontransfer/imagetransfer.h',
            'visiontransfer/asynctransfer.h',
            'visiontransfer/deviceenumeration.h',
            'visiontransfer/deviceinfo.h',
            'visiontransfer/sensordata.h',
            'visiontransfer/datachannelservice.h',
            'visiontransfer/reconstruct3d.h',
            ]:
            d.generate(basedir, filename)

        d.store_docstrings_to_file('visiontransfer_src/visiontransfer_docstrings_autogen.py')
    else:
        print("Could not open library base dir, please set a correct LIBVISIONTRANSFER_SRCDIR")

