#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Digraph
{
public:
    Digraph() = default;

    void reset_graph_size(std::size_t size);

    void add_edge(unsigned v, unsigned w);

    std::vector<unsigned> get_neighbours(unsigned v) const;

    std::size_t size() const
    {
        return graph.size();
    }

    void print(std::ostream & out) const;

private:
    friend class ShortestCommonAncestor;

    std::vector<std::vector<unsigned>> graph;           // vert -> vert, vert, ...
    std::unordered_map<unsigned, unsigned> id_vert_map; // id -> vert
    std::vector<unsigned> vert_id_map;                  // vert -> id

    unsigned get_or_add_vert(unsigned v); // id -> vert
};

inline std::ostream & operator<<(std::ostream & out, const Digraph & d)
{
    d.print(out);
    return out;
}

class ShortestCommonAncestor
{
    friend class WordNet;

    const Digraph & graph;

    explicit ShortestCommonAncestor(const Digraph & dg)
        : graph(dg)
    {
    }

    std::pair<unsigned, unsigned> ancestor_length(const std::vector<unsigned> & subset_a,
                                                  const std::vector<unsigned> & subset_b) const;

    // calculates length of shortest common ancestor path from node with id 'v' to node with id 'w'
    unsigned length(unsigned v, unsigned w)
    {
        return ancestor_length({v}, {w}).second;
    }

    // returns node id of shortest common ancestor of nodes v and w
    unsigned ancestor(unsigned v, unsigned w)
    {
        return ancestor_length({v}, {w}).first;
    }

    // calculates length of shortest common ancestor path from node subset 'subset_a' to node subset 'subset_b'
    unsigned length_subset(const std::set<unsigned> & subset_a, const std::set<unsigned> & subset_b)
    {
        return ancestor_length({subset_a.begin(), subset_a.end()},
                               {subset_b.begin(), subset_b.end()})
                .second;
    }

    // returns node id of shortest common ancestor of node subset 'subset_a' and node subset 'subset_b'
    unsigned ancestor_subset(const std::set<unsigned> & subset_a, const std::set<unsigned> & subset_b)
    {
        return ancestor_length({subset_a.begin(), subset_a.end()},
                               {subset_b.begin(), subset_b.end()})
                .first;
    }
};

class WordNet
{
    using storage_type = std::unordered_map<std::string_view, std::vector<unsigned>>;

public:
    WordNet(std::istream & synsets, std::istream & hypernyms);

    /**
     * Simple proxy class used to enumerate nouns.
     *
     * Usage example:
     *
     * WordNet wordnet{...};
     * ...
     * for (const std::string & noun : wordnet.nouns()) {
     *     // ...
     * }
     */
    class Nouns
    {
        friend class WordNet;

        const storage_type & storage;

        Nouns(const storage_type & storage)
            : storage(storage)
        {
        }

    public:
        class iterator
        {
        public:
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::string_view;
            using pointer = const value_type *;
            using reference = const value_type &;

            iterator() = default;

            bool operator==(const iterator & rhs) const
            {
                return it == rhs.it;
            }

            bool operator!=(const iterator & rhs) const
            {
                return !(*this == rhs);
            }

            reference operator*() const
            {
                return (*it).first;
            }

            pointer operator->() const
            {
                return &it->first;
            }

            iterator & operator++()
            {
                ++it;
                return *this;
            }

            iterator operator++(int)
            {
                auto tmp = *this;
                operator++();
                return tmp;
            }

        private:
            friend class Nouns;

            using wrapped_iterator = storage_type::const_iterator;

            iterator(wrapped_iterator it)
                : it(it)
            {
            }

            wrapped_iterator it;
        };

        iterator begin() const
        {
            return iterator(storage.begin());
        }
        iterator end() const
        {
            return iterator(storage.end());
        }
    };

    // lists all nouns stored in WordNet
    Nouns nouns() const
    {
        return Nouns(word_ids);
    }

    // returns 'true' if 'word' is stored in WordNet
    bool is_noun(const std::string & word) const
    {
        return word_ids.find(word) != word_ids.end();
    }

    // returns gloss of "shortest common ancestor" of noun1 and noun2
    std::string sca(const std::string & noun1, const std::string & noun2) const
    {
        return std::string(glosses.at(find_sca_distance(noun1, noun2).first));
    }

    // calculates distance between noun1 and noun2
    unsigned distance(const std::string & noun1, const std::string & noun2) const
    {
        return find_sca_distance(noun1, noun2).second;
    }

private:
    std::vector<std::string> source_lines;
    storage_type word_ids;
    std::unordered_map<unsigned, std::string_view> glosses;
    Digraph graph;

    std::pair<unsigned, unsigned> find_sca_distance(const std::string & noun1, const std::string & noun2) const
    {
        return ShortestCommonAncestor(graph).ancestor_length(
                word_ids.at(noun1),
                word_ids.at(noun2));
    }
};

class Outcast
{
public:
    explicit Outcast(WordNet & wordnet)
        : wordnet(wordnet)
    {
    }

    // returns outcast word
    std::string outcast(const std::set<std::string> & nouns) const;

private:
    WordNet & wordnet;
};
