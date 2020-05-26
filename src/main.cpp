/**
 * @author Hassan Nikaein
 */

#include <iostream>
#include <unordered_map>
#include <set>
#include <fstream>
#include <cstring>
#include <getopt.h>
#include "node.h"
#include "utils/logger.h"

using namespace std;


#define MAX_NODES                   100000000
#define MAX_SEQ_SIZE                50000

extern Logger *logger;
int log_level = Logger::INFO, merge_type = 0, k = -1, statistics = 0;
int max_node_ids = -1; // For debugging purposes
bool unify_before_run = false;
unsigned long sequences_last_unfill;
char *input_file_name, *output_file_name, *sequences[MAX_NODES],
        *help_str = const_cast<char *>("stark v1.0\nUsage: stark -i input_file_name [-o output_file_name] "
                                       "[-m merge_type] [-l log_level] [-u] [-s statistics-level]\n\n"
                                       "    -i,      --input=FILE           use FILE for input\n"
                                       "    -o,      --output=FILE          use FILE for output\n"
                                       "    -l,      --log=LEVEL            use LEVEL for log level (0=OFF, 1000=ALL)\n"
                                       "    -m,      --merge-type=TYPE      use TYPE for merging (0=no merge, "
                                       "1=only node reducing merges, 2=all merges)\n"
                                       "    -u,      --unify-before-run     unify input file unitigs before use\n"
                                       "    -s,      --statistics=TYPE      print statistics (0=no statistics, "
                                       "1=trivial statistics, 2=cpu-consuming statistics)\n\n"
);


void read_gfa() {
    unordered_map<string, long> node_ids;
    vector<tuple<string, char, string, char>> late_edges;
    char command, from_sign, to_sign, buf[MAX_SEQ_SIZE];
    string node_name, trash, from_name, to_name;
    int version = 1;
    long from_id, to_id;
    size_t seq_len;

    logger->debug("reading gfa file: %s", input_file_name);
    ifstream ifs(input_file_name);
    while ((command = static_cast<char>(ifs.get())) != EOF) {
        int discard = 0;
        switch (command) {
            case 'H':
                ifs >> buf;
                if (memcmp(buf, "VN", 2) == 0)
                    version = static_cast<int>(strtol(buf + 5, nullptr, 10));
                else
                    discard = 2;
                break;
            case 'S':
                ifs >> node_name >> buf;
                if (version == 2 || isdigit(buf[0])) {
                    version = 2;
                    ifs >> buf;
                }
                if (max_node_ids == -1 || Node::last_id < max_node_ids) {
                    seq_len = strlen(buf);
                    sequences[sequences_last_unfill] = new char[seq_len + 1];
                    memcpy(sequences[sequences_last_unfill], buf, seq_len + 1);
                    node_ids[node_name] = Node::add_node(sequences[sequences_last_unfill++], static_cast<int>(seq_len));
                }
                break;
            case 'E':
            case 'L':
                int match;
                if (command == 'E')
                    ifs >> trash;
                ifs >> from_name >> from_sign >> to_name >> to_sign;
                if (command == 'E') {
                    to_name = from_sign;
                    from_sign = from_name.back();
                    to_sign = to_name.back();
                    from_name.pop_back();
                    to_name.pop_back();
                    ifs >> trash >> trash;
                }
                ifs >> match;
                if (k == -1)
                    k = match + 1;
                if (k != match + 1)
                    logger->error("Error! different k's: %d - %d", k, match + 1);
                if (node_ids.find(from_name) != node_ids.end() && node_ids.find(to_name) != node_ids.end()) {
                    from_id = node_ids[from_name];
                    to_id = node_ids[to_name];
                    Node::add_edge(from_id, from_sign, to_id, to_sign);
                } else if (max_node_ids == -1 || Node::last_id < max_node_ids)
                    late_edges.emplace_back(from_name, from_sign, to_name, to_sign);
                break;
            default:
                discard = 1;
        }
        ifs.getline(buf + (discard == 2 ? strlen(buf) : 0), sizeof(buf));
        if (discard)
            logger->warn("line not supported: %c %s", command, buf);
    }
    ifs.close();
    for (auto &late_edge : late_edges) {
        tie(from_name, from_sign, to_name, to_sign) = late_edge;
        if (node_ids.find(from_name) == node_ids.end() || node_ids.find(to_name) == node_ids.end()) {
            if (max_node_ids == -1)
                logger->warn("Undefined node: %d -> %d!", from_name.c_str(), to_name.c_str());
            continue;
        }
        from_id = node_ids[from_name];
        to_id = node_ids[to_name];
        Node::add_edge(from_id, from_sign, to_id, to_sign);
    }
    node_ids.clear();
    late_edges.clear();
    logger->debug("read completed!");
}


long count_edges() {
    long total_degrees = 0;
    for (auto &node: Node::nodes)
        total_degrees += node.second.left_edges.size() + node.second.right_edges.size();
    return total_degrees / 2;
}


long count_deadends() {
    long total_deadends = 0;
    for (auto &node: Node::nodes) {
        if (node.second.left_edges.empty())
            total_deadends++;
        if (node.second.right_edges.empty())
            total_deadends++;
    }
    return total_deadends;
}

void print_statistics(int cur_k) {
    if (statistics == 0)
        return;
    logger->info("total_nodes: %ld", Node::nodes.size());
    if (statistics == 2) {
        long total_edges = count_edges();
        long total_not_unified_nodes = Node::nodes.size();
        long total_letters = 0;
        for (auto &node: Node::nodes) {
            if (node.second.sequence_len < cur_k) {
                logger->fatal("ERROR in cur_k during statistics!");
                break;
            }
            total_not_unified_nodes += node.second.sequence_len - cur_k;
            total_letters += node.second.sequence_len;
        }
        long total_not_unified_edges = total_edges + total_not_unified_nodes;
        total_not_unified_edges -= Node::nodes.size();
        logger->debugl2("total_edges: %ld", total_edges);
        logger->debug("total_nodes (expanded): %ld", total_not_unified_nodes);
        logger->debugl2("total_edges (expanded): %ld", total_not_unified_edges);
        logger->debugl2("total_deadends: %ld", count_deadends());
        logger->debug("total_letters: %ld", total_letters);
    }
}

void bluntify() {
    logger->debug("bluntifying graph");
    for (long i = 1; i <= Node::last_id; ++i) {
        if (Node::nodes.find(i) == Node::nodes.end())
            continue;
        Node &node = Node::nodes.find(i)->second;
        int from, to;
        if (!node.left_edges.empty())
            from = (k - 1) / 2;
        else
            from = 0;
        if (!node.right_edges.empty())
            to = node.sequence_len - k / 2;
        else
            to = node.sequence_len;
        node.set_sequence(node.get_sequence() + from, to - from);
    }
    if (k % 2 == 0) {
        set<pair<long, long>> good_edges;
        long node_last_id = Node::last_id;
        for (long i = 1; i <= node_last_id; ++i) {
            if (Node::nodes.find(i) == Node::nodes.end())
                continue;
            Node &node = Node::nodes.find(i)->second;
            long new_right_node_id = 0;
            auto right_edges = node.right_edges;
            for (long right_neighbour_id : right_edges) {
                if (right_neighbour_id > 0) {
                    if (new_right_node_id == 0) {
                        new_right_node_id = Node::add_node(node.get_sequence() + node.sequence_len, 1);
                        Node::add_edge(node.id, '+', new_right_node_id, '+');
                    }
                    node.right_edges.erase(right_neighbour_id);
                    Node::nodes.find(right_neighbour_id)->second.right_edges.erase(node.id);
                    Node::add_edge(new_right_node_id, '+', right_neighbour_id, '-');
                }
            }
            auto left_edges = node.left_edges;
            for (long left_neighbour_id : left_edges)
                if (left_neighbour_id < 0 &&
                    good_edges.find(pair<long, long>(-1 * left_neighbour_id, node.id)) == good_edges.end()) {
                    Node &left_neighbour = Node::nodes.find(-1 * left_neighbour_id)->second;
                    node.left_edges.erase(left_neighbour_id);
                    left_neighbour.left_edges.erase(node.id * -1);
                    long left_neighbour_right_edge_size =
                            left_neighbour.sequence_len > 1 ? 1 : left_neighbour.right_edges.size();
                    long node_right_edge_size = node.sequence_len > 1 ? 1 : node.right_edges.size();
                    if (left_neighbour_right_edge_size == 0 || node_right_edge_size == 0)
                        continue;
                    Node *from_node, *to_node;
                    if (left_neighbour_right_edge_size < node_right_edge_size) {
                        from_node = &left_neighbour;
                        to_node = &node;
                    } else {
                        from_node = &node;
                        to_node = &left_neighbour;
                    }
                    if (from_node->sequence_len > 1) {
                        long expanded_node_id = Node::add_node(from_node->get_sequence() + 1,
                                                               from_node->sequence_len - 1);
                        from_node->sequence_len = 1;
                        from_node->move_right_edges_to(Node::nodes.find(expanded_node_id)->second);
                        Node::add_edge(from_node->id, '+', expanded_node_id, '+');
                    }
                    for (long right_neighbour_id : from_node->right_edges)
                        if (right_neighbour_id < 0) {
                            Node::add_edge(-1 * right_neighbour_id, '-', to_node->id, '+');
                            good_edges.emplace(pair<long, long>(-1 * right_neighbour_id, to_node->id));
                            good_edges.emplace(pair<long, long>(to_node->id, -1 * right_neighbour_id));
                        } else
                            Node::add_edge(right_neighbour_id, '+', to_node->id, '+');
                }
        }
    }
}

void unify(int cur_k) {
    logger->debug("unifying");
    int cur_k_1 = cur_k - 1;
    for (long i = 1; i <= Node::last_id; ++i) {
        if (Node::nodes.find(i) == Node::nodes.end())
            continue;
        Node &node = Node::nodes.find(i)->second;
        if (node.left_edges.size() != 1)
            continue;
        long left_neighbour_id = node.left_edges.front();
        if (left_neighbour_id < 0)
            continue;
        Node &left_neighbour = Node::nodes.find(left_neighbour_id)->second;
        if (left_neighbour.right_edges.size() != 1)
            continue;
        if (left_neighbour.id == node.id)
            continue;
        node.move_right_edges_to(left_neighbour, false);
        char *after_left_neighbour_sequence = left_neighbour.get_sequence() + left_neighbour.sequence_len;
        bool new_char_needed = false;
        for (int j = cur_k_1; j < node.sequence_len; ++j)
            if (after_left_neighbour_sequence[j - cur_k_1] != node.get_sequence()[j]) {
                new_char_needed = true;
                break;
            }
        if (!new_char_needed)
            left_neighbour.sequence_len += node.sequence_len - cur_k_1;
        else {
            sequences[sequences_last_unfill] = new char[left_neighbour.sequence_len + node.sequence_len - cur_k_1 + 1];
            memcpy(sequences[sequences_last_unfill], left_neighbour.get_sequence(),
                   static_cast<size_t>(left_neighbour.sequence_len));
            memcpy(sequences[sequences_last_unfill] + left_neighbour.sequence_len, node.get_sequence() + cur_k_1,
                   static_cast<size_t>(node.sequence_len - cur_k_1));
            sequences[sequences_last_unfill][left_neighbour.sequence_len + node.sequence_len - cur_k_1] = 0;
            left_neighbour.set_sequence(sequences[sequences_last_unfill],
                                        left_neighbour.sequence_len + node.sequence_len - cur_k_1);
            sequences_last_unfill++;
        }
        Node::nodes.erase(node.id);
    }
}

void merge_nodes(bool growing_merge = false) {
    logger->debug("merging");
//    unordered_map<char, Node *> end_right_nodes;
//    unordered_map<char, Node *> end_left_nodes;
//    for (long i = 1; i <= Node::last_id; ++i) {
//        if (Node::nodes.find(i) == Node::nodes.end())
//            continue;
//        Node &node = Node::nodes.find(i)->second;
//        if (node.right_edges.empty()) {
//            const auto &end_right_node = end_right_nodes.find(node.get_sequence()[node.sequence_len - 1]);
//            if (end_right_node == end_right_nodes.end() ||
//                node.partial_right_merge_to(*end_right_node->second, growing_merge) == node.id)
//                end_right_nodes[node.get_sequence()[node.sequence_len - 1]] = &node;
//        }
//        if (node.left_edges.empty()) {
//            const auto &end_left_node = end_left_nodes.find(node.get_sequence()[0]);
//            if (end_left_node == end_left_nodes.end() ||
//                node.partial_left_merge_to(*end_left_node->second, growing_merge) == node.id)
//                end_left_nodes[node.get_sequence()[0]] = &node;
//        }
//    }
    long min_change_per_step = Node::last_id / 1000;
    long changed = min_change_per_step + 1;
    int step = 0;
    while (changed > min_change_per_step) {
        changed = 0;
        logger->info("Try to merge step %d for %ld nodes", step, Node::nodes.size());
        unify(1);
        long i_debug_step = Node::last_id / 10;
        for (long i = 1; i <= Node::last_id; ++i) {
            if (i % i_debug_step == 0)
                logger->debugl3("merge i: %d", i);
            if (Node::nodes.find(i) == Node::nodes.end())
                continue;
            Node &node = Node::nodes.find(i)->second;
            set<long> neighbours;
            if (!node.left_edges.empty()) {
                neighbours.insert(node.left_edges.front());
                neighbours.insert(node.left_edges.back());
            }
            if (!node.right_edges.empty()) {
                neighbours.insert(node.right_edges.front());
                neighbours.insert(node.right_edges.back());
            }
            Edges candidates;
            for (auto &neighbour_id : neighbours) {
                auto &neighbour = Node::nodes.find(abs(neighbour_id))->second;
                if (neighbour_id < 0)
                    candidates.merge_with(neighbour.left_edges);
                else
                    candidates.merge_with(neighbour.right_edges);
            }
            for (auto candidate_id : candidates) {
                Node &candidate_node = Node::nodes.find(abs(candidate_id))->second;
                if (candidate_node.id == i)
                    continue;
                if (candidate_node.left_edges.find(candidate_node.id))
                    continue;
                if (candidate_node.left_edges.find(-1 * candidate_node.id))
                    continue;
                if (candidate_node.right_edges.find(candidate_node.id))
                    continue;
                if (candidate_node.right_edges.find(-1 * candidate_node.id))
                    continue;
                if ((candidate_node.left_edges == node.left_edges &&
                     candidate_node.partial_left_merge_to(node, growing_merge)) ||
                    (candidate_node.right_edges == node.right_edges &&
                     candidate_node.partial_right_merge_to(node, growing_merge))) {
                    logger->debugl4("%d\t%.*s\n%d\t%.*s\n\n", i, node.sequence_len, node.get_sequence(),
                                    candidate_node.id, candidate_node.sequence_len, candidate_node.get_sequence());
                    changed++;
                    break;
                }
            }
        }
        step++;
    }
    unify(1);
}

void write_to_file(const char *file_name) {
    logger->debug("writing results!");
    ofstream ofs(file_name);
    for (auto &node : Node::nodes) {
        ofs << "S\t" << node.second.id << "\t";
        ofs.write(node.second.get_sequence(), node.second.sequence_len);
        ofs << "\n";
    }
    for (auto &node : Node::nodes) {
        for (long left_neighbour_id : node.second.left_edges)
            ofs << "L\t" << node.second.id << "\t-\t" << abs(left_neighbour_id) << "\t"
                << (left_neighbour_id < 0 ? "+" : "-") << "\t0M\n";
        for (long right_neighbour_id : node.second.right_edges)
            ofs << "L\t" << node.second.id << "\t+\t" << abs(right_neighbour_id) << "\t"
                << (right_neighbour_id < 0 ? "+" : "-") << "\t0M\n";
    }
    ofs.close();
    logger->debug("write completed!");
}

int read_args(int argc, char *argv[]) {
    static struct option long_options[] =
            {
                    {"input",            required_argument, nullptr, 'i'},
                    {"output",           required_argument, nullptr, 'o'},
                    {"log",              required_argument, nullptr, 'l'},
                    {"merge-type",       required_argument, nullptr, 'm'},
                    {"unify-before-run", no_argument,       nullptr, 'u'},
                    {"statistics",       required_argument, nullptr, 's'},
            };

    int option_index = 0, c;
    bool need_help = false;
    while ((c = getopt_long(argc, argv, "i:o:l:m:us:", long_options, &option_index)) >= 0)
        switch (c) {
            case 'i':
                input_file_name = strdup(optarg);
                break;
            case 'o':
                output_file_name = strdup(optarg);
                break;
            case 'l':
                log_level = static_cast<int>(strtol(optarg, nullptr, 10));
                break;
            case 'm':
                merge_type = static_cast<int>(strtol(optarg, nullptr, 10));
                break;
            case 'u':
                unify_before_run = true;
                break;
            case 's':
                statistics = static_cast<int>(strtol(optarg, nullptr, 10));
                break;
            default:
                need_help = true;
                break;
        }
    logger = new Logger(log_level);
    if (!input_file_name)
        need_help = true;
    if (need_help) {
        cout << help_str << endl;
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
//    input_file_name = strdup("data/human_63.gfa");
//    log_level = Logger::DEBUGL2;
//    merge_type = 1;
//    statistics = 2;
//    max_node_ids = 10000;
    if (read_args(argc, argv))
        return 1;
    read_gfa();
    print_statistics(k);
    if (unify_before_run) {
        unify(k);
        print_statistics(k);
    }
    bluntify();
    print_statistics(1);
    if (k % 2 == 0) {
        unify(1);
        print_statistics(1);
    }
    if (merge_type > 0) {
        merge_nodes(merge_type == 2);
        print_statistics(1);
    }
    if (output_file_name)
        write_to_file(output_file_name);
}