# -*- coding: utf-8 -*-
from __future__ import unicode_literals
from __future__ import absolute_import
from __future__ import print_function

import json


def _convert_key(s):
    if any(ord(c) > 127 for c in s):
        return json.dumps(s)[1:-1]
    return s


def consume_string(line, index):
    index += 1
    while line[index] != '"':
        if line[index] == '\\':
            index += 1
        index += 1

    return index


def scan_value(line, index):
    term_chars = "]},"
    # Check if we're doing a recursive object or not
    if line[index] == '{':
        recursive = True
        delims = "{}"
    elif line[index] == '[':
        recursive = True
        delims = "[]"
    else:
        recursive = False

    if recursive:
        depth = 0
        while True:
            if line[index] == delims[0]:
                depth += 1
            elif line[index] == delims[1]:
                depth -= 1
                if depth == 0:
                    # Increment index off closing marker
                    index += 1
                    break
            elif line[index] == '"':
                index = consume_string(line, index)
            index += 1
    else:
        while True:
            if index == len(line) or line[index] in term_chars:
                break
            if line[index] == '"':
                index = consume_string(line, index)
            index += 1
    return index


def scan_key(line, key):
    index = 0
    while True:
        index = line.find(key, index)
        if index == -1:
            break
        # Check that we're surrounded by "<key>":
        # with any amount of whitespace b/w closing quote and colon
        if not (line[index - 1] == '"' and line[index - 2] != '\\' and line[index + len(key)] == '"'):
            index += 1
            continue

        # Move up and find the
        index += len(key) + 1
        while line[index].isspace():
            index += 1

        # assert we are on a colon
        if line[index] != ':':
            continue

        # Move up to the data element
        index += 1
        while line[index].isspace():
            index += 1

        return index

    raise KeyError(key)


def extract(line, key, tokenize=True):
    key = _convert_key(key)
    # Only tokenize if we need to
    data = line
    if tokenize and key.find('.') > 0:
        for subkey in key.split('.'):
            s_idx = scan_key(data, subkey)
            e_idx = scan_value(data, s_idx)
            data = data[s_idx:e_idx]
        return json.loads(data)
    else:
        s_idx = scan_key(data, key)
        e_idx = scan_value(data, s_idx)
        return json.loads(data[s_idx:e_idx])


def pluck_list(line, key, index):

    if index < 0:
        raise IndexError
    key = _convert_key(key)

    list_start = scan_key(line, key)

    if line[list_start] != '[':
        raise KeyError(key)

    list_end = scan_value(line, list_start)
    l = line[list_start:list_end]

    pos = 1
    while l[pos].isspace():
        pos += 1

    if l[pos] == ']':
        raise IndexError

    while index > 0:
        pos = scan_value(l, pos)

        if l[pos] == ',':
            pos += 1
        while l[pos].isspace():
            pos += 1
        if l[pos] == ']':
            raise IndexError
        index -= 1

    end = scan_value(l, pos)
    return json.loads(l[pos:end])

def key_exists(line, key, tokenize=True):

    key = _convert_key(key)
    data = line
    try:
        if tokenize and key.find('.') > 0:
            for subkey in key.split('.'):
                s_idx = scan_key(data, subkey)
                e_idx = scan_value(data, s_idx)
                # TODO: We should use integer scoping over
                # string extraction since CPython creates
                # a copy on non-trivial slices (anything not
                # all or nothing slice).
                data = data[s_idx:e_idx]
        else:
            s_idx = scan_key(data, key)
            e_idx = scan_value(data, s_idx)
    except (KeyError, IndexError):
        return False
    return True
    
    
def extract_all(line, key):
    # TODO: Implement iterative extraction
    raise NotImplemented
