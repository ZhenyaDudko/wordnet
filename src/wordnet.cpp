#include "wordnet.h"

#include <charconv>
#include <istream>
#include <limits>
#include <ostream>
#include <queue>

namespace {

unsigned string_view_to_unsigned(std::string_view sv)
{
    unsigned result;
    std::from_chars(sv.begin(), sv.end(), result);
    return result;
}

} // anonymous namespace

void Digraph::reset_graph_size(std::size_t size)
{
    graph.reserve(size);
    vert_id_map.reserve(size);
    id_vert_map.reserve(size);
}

void Digraph::add_edge(unsigned v, unsigned w)
{
    graph[get_or_add_vert(v)].emplace_back(get_or_add_vert(w));
}

std::vector<unsigned> Digraph::get_neighbours(unsigned v) const
{
    auto found_vertex = id_vert_map.find(v);
    if (found_vertex != id_vert_map.end()) {
        std::vector<unsigned> neighbours = graph[found_vertex->second];
        for (unsigned & n : neighbours) {
            n = vert_id_map[n];
        }
        return neighbours;
    }
    return {};
}

void Digraph::print(std::ostream & out) const
{
    out << "vertex: its neighbours\n";
    for (std::size_t i = 0; i < graph.size(); ++i) {
        out << vert_id_map[i] << ": ";
        for (std::size_t j = 0; j < graph[i].size(); ++j) {
            out << vert_id_map[graph[i][j]] << " ";
        }
        out << '\n';
    }
}

unsigned Digraph::get_or_add_vert(unsigned v) // id -> vert
{
    auto [it, inserted] = id_vert_map.emplace(v, vert_id_map.size());
    if (inserted) {
        vert_id_map.emplace_back(v);
        graph.emplace_back();
    }
    return it->second;
}

std::pair<unsigned, unsigned> ShortestCommonAncestor::ancestor_length(const std::vector<unsigned> & subset_a,
                                                                      const std::vector<unsigned> & subset_b) const
{
    std::vector<unsigned> distance(graph.size());
    std::vector<char> color(graph.size());
    std::queue<unsigned> queue;
    for (const unsigned id : subset_a) {
        unsigned vert = graph.id_vert_map.at(id);
        color[vert] = 1;
        queue.emplace(vert);
    }
    for (const unsigned id : subset_b) {
        unsigned vert = graph.id_vert_map.at(id);
        if (color[vert] == 1) {
            return {id, 0};
        }
        color[vert] = 2;
        queue.emplace(vert);
    }
    unsigned min_distance = std::numeric_limits<unsigned>::max();
    unsigned min_distance_ancestor = 0;
    while (!queue.empty()) {
        const unsigned vert = queue.front();
        queue.pop();
        for (const unsigned to : graph.graph[vert]) {
            if (color[to] == 0) {
                color[to] = color[vert];
                distance[to] = distance[vert] + 1;
                queue.emplace(to);
            }
            else if (color[to] != color[vert]) {
                unsigned current_distance = distance[to] + distance[vert] + 1;
                if (current_distance < min_distance) {
                    min_distance = current_distance;
                    min_distance_ancestor = to;
                }
            }
        }
    }
    return {graph.vert_id_map[min_distance_ancestor], min_distance};
}

WordNet::WordNet(std::istream & synsets, std::istream & hypernyms)
{
    std::string line;
    while (std::getline(synsets, line)) {
        if (line.empty()) {
            continue;
        }
        source_lines.emplace_back(std::move(line));
    }
    for (const std::string & source_line : source_lines) {
        std::string_view id_string(&source_line[0], source_line.find(','));
        unsigned id = string_view_to_unsigned(id_string);
        std::size_t gloss_start = source_line.find(',', id_string.size() + 1) + 1;
        std::size_t start_pos = id_string.size() + 1;
        std::size_t end_pos;
        while ((end_pos = source_line.find(' ', start_pos)) < gloss_start) {
            word_ids[{&source_line[start_pos], end_pos - start_pos}].emplace_back(id);
            start_pos = end_pos + 1;
        }
        word_ids[{&source_line[start_pos], gloss_start - 1 - start_pos}].emplace_back(id);
        glosses[id] = {&source_line[gloss_start], source_line.size() - gloss_start};
    }

    graph.reset_graph_size(source_lines.size());
    while (std::getline(hypernyms, line)) {
        if (line.empty()) {
            continue;
        }
        std::string_view from_string(&line[0], line.find(','));
        if (from_string.size() < line.size()) {
            unsigned from = string_view_to_unsigned(from_string);
            std::size_t start_pos = from_string.size() + 1;
            std::size_t end_pos;
            while ((end_pos = line.find(',', start_pos)) < line.size()) {
                graph.add_edge(from, string_view_to_unsigned({&line[start_pos], end_pos - start_pos}));
                start_pos = end_pos + 1;
            }
            graph.add_edge(from, string_view_to_unsigned({&line[start_pos], line.size() - start_pos}));
        }
    }
}

std::string Outcast::outcast(const std::set<std::string> & nouns) const
{
    if (nouns.size() <= 2) {
        return "";
    }
    std::vector<unsigned> distances(nouns.size());
    std::size_t answer_word_number = 0;
    std::set<std::string>::iterator answer_word_iterator;
    bool max_dist_was_repeated = false;
    std::size_t first_pos = 0;
    for (auto i = nouns.begin(); i != nouns.end(); ++i, ++first_pos) {
        std::size_t second_pos = first_pos + 1;
        for (auto j = std::next(i); j != nouns.end(); ++j, ++second_pos) {
            unsigned distance = wordnet.distance(*i, *j);
            distances[first_pos] += distance;
            distances[second_pos] += distance;
        }
        if (distances[first_pos] > distances[answer_word_number] || first_pos == 0) {
            answer_word_number = first_pos;
            answer_word_iterator = i;
            max_dist_was_repeated = false;
        }
        else if (distances[first_pos] == distances[answer_word_number]) {
            max_dist_was_repeated = true;
        }
    }
    return (max_dist_was_repeated ? "" : *answer_word_iterator);
}
