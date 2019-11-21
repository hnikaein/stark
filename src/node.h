/**
 * @author Hassan Nikaein
 */

#include <unordered_map>
#include <vector>

using namespace std;

#ifndef STARK_NODE_H
#define STARK_NODE_H


class Edges {
private:
    class EdgesIterator {
    private:
        Edges *edges;
        int index;
    public:
        EdgesIterator() = delete;

        explicit EdgesIterator(Edges *edges, int index);

        bool operator!=(const EdgesIterator &edgesIterator);

        void operator++();

        long operator*();
    };

    vector<long> neighbour_ids;
public:
    bool empty();

    long size();

    void erase(long id);

    void insert(long id);

    void clear();

    void merge_with(const Edges &another_edges);

    bool find(long id);

    long front();

    long back();

    long get(int index);

    EdgesIterator begin();

    EdgesIterator end();

    bool operator==(Edges &another_edges);
};

class Node {
public:
    static long last_id;
    static unordered_map<long, Node> nodes;

    long id;
    int sequence_len;
    Edges left_edges;
    Edges right_edges;

    explicit Node(char *sequence, int sequence_len, long left_neighbour_id = 0, long right_neighbour_id = 0);

    Node() = delete;

    void set_sequence(char *sequence, int sequence_len);

    char *get_sequence();

    void merge_to(Node &node);

    long partial_left_merge_to(Node &node, bool growing_merge);

    long partial_right_merge_to(Node &node, bool growing_merge);

    void move_right_edges_to(Node &node, bool update = true);

    static long add_node(char *sequence, int sequence_len, long left_neighbour_id = 0, long right_neighbour_id = 0);

    static void add_edge(long from_node_id, char from_side, long to_node_id, char to_side);

private:
    char *sequence;

    void move_left_edges_to(Node &node, bool update = true);
};


#endif //STARK_NODE_H
