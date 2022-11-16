#!/usr/bin/env python
import networkx as nx
import scipy as sp
import scipy.io
import io
import sys


def main(argv):
    if len(argv) != 3:
        print('mm.py [in_graph_file] [out_graph_file]')
        return
    in_graph = argv[1]
    out_mm = argv[2]
    fh = io.BytesIO()
    G = nx.read_edgelist(in_graph, create_using=nx.Graph(), nodetype=int)
    print('read finished')
    m = nx.to_scipy_sparse_array(G, format='coo')
    sp.io.mmwrite(fh, m)
    with open(out_mm, 'wb') as f:
        f.write(fh.getbuffer())
    # with open('test.txt', 'w') as f:
    #     for i, j in length.items():
    #         f.write(f'{i} {j}\n')


if __name__ == '__main__':
    main(sys.argv)
