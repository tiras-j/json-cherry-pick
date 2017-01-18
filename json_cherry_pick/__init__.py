try:
    from json_cherry_pick.c_jcp import extract
    from json_cherry_pick.c_jcp import extract_all
    from json_cherry_pick.c_jcp import pluck_list
except ImportError:
    from json_cherry_pick.py_jcp import extract
    from json_cherry_pick.py_jcp import extract_all
    from json_cherry_pick.py_jcp import pluck_list

