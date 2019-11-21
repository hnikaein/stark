/**
 * @author Hassan Nikaein
 */

#include <cstring>
#include <algorithm>
#include <iostream>
#include "node.h"

Edges::EdgesIterator::EdgesIterator(Edges *edges, int index) : edges(edges) {
    this->index = static_cast<int>((index == -1) ? edges->size() : index);
}

bool Edges::EdgesIterator::operator!=(const Edges::EdgesIterator &edgesIterator) {
    return (edges != edgesIterator.edges) || (index != edgesIterator.index);
}

void Edges::EdgesIterator::operator++() {
    index++;
}

long Edges::EdgesIterator::operator*() {
    return edges->get(index);
}


bool Edges::empty() {
    return neighbour_ids.empty();
}

long Edges::size() {
    return neighbour_ids.size();
}

void Edges::erase(long id) {
    long edges_size = neighbour_ids.size();
    for (int i = 0; i < edges_size; ++i)
        if (neighbour_ids[i] == id) {
            neighbour_ids[i] = neighbour_ids.back();
            neighbour_ids.pop_back();
            break;
        }
}

void Edges::insert(long id) {
    if (find(id))
        return;
    neighbour_ids.push_back(id);
}

void Edges::clear() {
    neighbour_ids.clear();
}

void Edges::merge_with(const Edges &another_edges) {
    neighbour_ids.reserve(neighbour_ids.size() + another_edges.neighbour_ids.size());
    neighbour_ids.insert(neighbour_ids.end(), another_edges.neighbour_ids.begin(), another_edges.neighbour_ids.end());
    sort(neighbour_ids.begin(), neighbour_ids.end());
    for (long i = neighbour_ids.size() - 2; i >= 0; --i) {
        if (neighbour_ids[i] == neighbour_ids[i + 1]) {
            neighbour_ids[i] = neighbour_ids.back();
            neighbour_ids.pop_back();
            i--;
        }
    }
}

bool Edges::find(long id) {
    for (long neighbour_id : neighbour_ids)
        if (neighbour_id == id)
            return true;
    return false;
}

long Edges::front() {
    return neighbour_ids.front();
}

long Edges::back() {
    return neighbour_ids.back();
}

long Edges::get(int index) {
    return neighbour_ids[index];
}

Edges::EdgesIterator Edges::begin() {
    return Edges::EdgesIterator(this, 0);
}

Edges::EdgesIterator Edges::end() {
    return Edges::EdgesIterator(this, -1);
}


bool Edges::operator==(Edges &another_edges) {
    unsigned long size = neighbour_ids.size();
    if (size != another_edges.neighbour_ids.size())
        return false;
    sort(neighbour_ids.begin(), neighbour_ids.end());
    sort(another_edges.neighbour_ids.begin(), another_edges.neighbour_ids.end());
    for (unsigned int i = 0; i < size; ++i)
        if (neighbour_ids[i] != another_edges.neighbour_ids[i])
            return false;
    return true;
}


long Node::last_id;
unordered_map<long, Node> Node::nodes;


Node::Node(char *sequence, int sequence_len,
           long left_neighbour_id, long right_neighbour_id) {
    if (right_neighbour_id != 0)
        right_edges.insert(right_neighbour_id);
    if (left_neighbour_id != 0)
        left_edges.insert(left_neighbour_id);
    id = ++Node::last_id;
    set_sequence(sequence, sequence_len);
}

long Node::add_node(char *sequence, int sequence_len, long left_neighbour_id, long right_neighbour_id) {
    long node_id = Node::last_id + 1;
    Node::nodes.emplace(piecewise_construct,
                        forward_as_tuple(node_id),
                        forward_as_tuple(sequence, sequence_len, left_neighbour_id, right_neighbour_id));
    return node_id;
}

void Node::add_edge(long from_node_id, char from_side, long to_node_id, char to_side) {
    long signed_from_node_id = from_side == '+' ? from_node_id : from_node_id * -1;
    long signed_to_node_id = to_side == '-' ? to_node_id : to_node_id * -1;
    Node &from_node = Node::nodes.find(from_node_id)->second;
    Node &to_node = Node::nodes.find(to_node_id)->second;
    if (from_side == '+')
        from_node.right_edges.insert(signed_to_node_id);
    else
        from_node.left_edges.insert(signed_to_node_id);
    if (to_side == '+')
        to_node.left_edges.insert(signed_from_node_id);
    else
        to_node.right_edges.insert(signed_from_node_id);

}

void Node::move_right_edges_to(Node &node, bool update) {
    if (not update)
        node.right_edges.clear();
    node.right_edges.merge_with(right_edges);
    for (const long &right_neighbour_id:right_edges)
        if (right_neighbour_id == id) {
            node.right_edges.erase(id);
            node.right_edges.insert(node.id);
        } else {
            Node &neighbour = Node::nodes.find(abs(right_neighbour_id))->second;
            if (right_neighbour_id < 0) {
                neighbour.left_edges.erase(id);
                neighbour.left_edges.insert(node.id);
            } else {
                neighbour.right_edges.erase(id);
                neighbour.right_edges.insert(node.id);
            }
        }
    right_edges.clear();
}

void Node::move_left_edges_to(Node &node, bool update) {
    if (not update)
        node.left_edges.clear();
    node.left_edges.merge_with(left_edges);
    for (const long &left_neighbour_id:left_edges)
        if (left_neighbour_id == -1 * id) {
            node.left_edges.erase(-1 * id);
            node.left_edges.insert(-1 * node.id);
        } else {
            Node &neighbour = Node::nodes.find(abs(left_neighbour_id))->second;
            if (left_neighbour_id < 0) {
                neighbour.left_edges.erase(-1 * id);
                neighbour.left_edges.insert(-1 * node.id);
            } else {
                neighbour.right_edges.erase(-1 * id);
                neighbour.right_edges.insert(-1 * node.id);
            }
        }
    left_edges.clear();
}

void Node::merge_to(Node &node) {
    move_right_edges_to(node);
    move_left_edges_to(node);
    Node::nodes.erase(id);
}

long Node::partial_left_merge_to(Node &node, bool growing_merge) {
    int i;
    for (i = 0; i < min(sequence_len, node.sequence_len); ++i)
        if (get_sequence()[i] != node.get_sequence()[i])
            break;
    if (i == 0)
        return 0;
    if (i == node.sequence_len)
        if (i == sequence_len) {
            merge_to(node);
            return node.id;
        } else {
            set_sequence(get_sequence() + i, sequence_len - i);
            move_left_edges_to(node);
            add_edge(node.id, '+', id, '+');
            return node.id;
        }
    else {
        if (i == sequence_len) {
            node.set_sequence(node.get_sequence() + i, node.sequence_len - i);
            node.move_left_edges_to(*this);
            add_edge(id, '+', node.id, '+');
            return id;
        } else if (growing_merge) {
            long new_node_id = Node::add_node(get_sequence(), i);
            Node &new_node = Node::nodes.find(new_node_id)->second;
            set_sequence(get_sequence() + i, sequence_len - i);
            node.set_sequence(node.get_sequence() + i, node.sequence_len - i);
            node.move_left_edges_to(new_node);
            move_left_edges_to(new_node);
            add_edge(new_node_id, '+', id, '+');
            add_edge(new_node_id, '+', node.id, '+');
            return new_node_id;
        } else
            return 0;
    }
}

long Node::partial_right_merge_to(Node &node, bool growing_merge) {
    int i;
    for (i = 1; i <= min(sequence_len, node.sequence_len); ++i)
        if (get_sequence()[sequence_len - i] != node.get_sequence()[node.sequence_len - i])
            break;
    i--;
    if (i == 0)
        return 0;
    if (i == node.sequence_len)
        if (i == sequence_len) {
            merge_to(node);
            return node.id;
        } else {
            sequence_len -= i;
            move_right_edges_to(node);
            add_edge(id, '+', node.id, '+');
            return node.id;
        }
    else {
        if (i == sequence_len) {
            node.sequence_len -= i;
            node.move_right_edges_to(*this);
            add_edge(node.id, '+', id, '+');
            return id;
        } else if (growing_merge) {
            long new_node_id = Node::add_node(get_sequence() + sequence_len - i, i);
            Node &new_node = Node::nodes.find(new_node_id)->second;
            sequence_len -= i;
            node.sequence_len -= i;
            node.move_right_edges_to(new_node);
            move_right_edges_to(new_node);
            add_edge(id, '+', new_node_id, '+');
            add_edge(node.id, '+', new_node_id, '+');
            return new_node_id;
        } else
            return 0;
    }
}

void Node::set_sequence(char *sequence, int sequence_len) {
    this->sequence = sequence;
    this->sequence_len = sequence_len;
}

char *Node::get_sequence() {
    return sequence;
}

