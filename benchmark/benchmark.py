# -*- coding: utf-8 -*-
from __future__ import print_function
from __future__ import unicode_literals

import json
import sys
import jcp
import time
import resource

LINE = None
ITER = 100000


def extract():
    KEY = "2\\\"<1bw@+fYdn~(P~"

    start = time.time()
    for _ in range(ITER):
        jcp.extract(LINE, KEY)
    tot = time.time() - start
    return {"extract": tot}


def extract_large():
    KEY = " 1nBx6QW(z)8+d"

    start = time.time()
    for _ in range(ITER):
        jcp.extract(LINE, KEY)
    tot = time.time() - start
    return {"extract_large": tot}


def pluck_10():
    KEY = " 1nBx6QW(z)8+d"

    start = time.time()
    for _ in range(ITER):
        jcp.pluck_list(LINE, KEY, 10)
    tot = time.time() - start
    return {"pluck_10": tot}


def all():
    results = {}
    # Fetch starting maxrss
    results['rss_start'] = resource.getrusage(resource.RUSAGE_SELF)[2]
    results.update(extract())
    results.update(extract_large())
    results.update(pluck_10())
    # Some time to allow recounting/gc work. Not sure if necessary
    # but better safe than panic-induced.
    time.sleep(0.5)
    results['rss_end'] = resource.getrusage(resource.RUSAGE_SELF)[2]
    return results


def calc_diff(a, b):
    return 100.0 * ((b - a) / a)


def result():
    GOOD = '\033[92m'
    BAD = '\033[93m'
    ENDC = '\033[0m'

    input = sys.stdin.read()
    base, new = map(json.loads, input.split('::'))
    diff = {}
    map(
        lambda x: diff.update({x: calc_diff(base[x], new[x])}),
        [k for k in base.keys() if not k.startswith('rss')]
    )
    for k, v in sorted(diff.iteritems()):
        print("BENCH::{0} {1:>12}{2:.2f}%{3}".format(
            k,
            GOOD if v <= 0 else BAD,
            v,
            ENDC
        ))
    print("")
    print("[BASE]: MEMORY USAGE")
    print("START_RSS: {0}".format(base.get('rss_start')))
    print("END_RSS: {0}".format(base.get('rss_end')))
    print("")
    print("[NEW]: MEMORY USAGE")
    print("START_RSS: {0}".format(new.get('rss_start')))
    print("END_RSS: {0}".format(new.get('rss_end')))


if __name__ == "__main__":
    with open('benchmark/fixtures/line') as f:
        LINE = f.read()
    try:
        data = globals()[sys.argv[1]]()
    except Exception as e:
        print(str(e))
        sys.exit(1)
    if data:
        print(json.dumps(data))
