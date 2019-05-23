#ifndef RANGRAPH_H
#define RANGRAPH_H

ASGraph* ran_graph(int, int);
bool cyclic_util(ASGraph*, int, std::map<uint32_t, bool>*, std::map<uint32_t, bool>*);
bool is_cyclic(ASGraph*);
#endif
