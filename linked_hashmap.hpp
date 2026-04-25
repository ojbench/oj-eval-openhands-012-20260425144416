
/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */
    
template<
	class Key,
	class T,
	class Hash = std::hash<Key>, 
	class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
	/**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::linked_hashmap as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;

private:
	// Node structure for both hash table and linked list
	struct Node {
		// Use char array to avoid construction
		alignas(value_type) char data_storage[sizeof(value_type)];
		Node* next;      // next in hash bucket
		Node* prev;      // previous in linked list
		Node* link_next; // next in linked list
		Node* link_prev; // previous in linked list
		bool is_dummy;   // flag for dummy nodes
		
		value_type* get_data() {
			return reinterpret_cast<value_type*>(data_storage);
		}
		
		const value_type* get_data() const {
			return reinterpret_cast<const value_type*>(data_storage);
		}
		
		Node(const value_type& val) : next(nullptr), prev(nullptr), link_next(nullptr), link_prev(nullptr), is_dummy(false) {
			new (data_storage) value_type(val);
		}
		
		Node() : next(nullptr), prev(nullptr), link_next(nullptr), link_prev(nullptr), is_dummy(true) {
			// Don't construct data for dummy nodes
		}
		
		~Node() {
			if (!is_dummy) {
				get_data()->~value_type();
			}
		}
	};
	
	// Hash table
	Node** table;
	size_t table_size;
	size_t element_count;
	
	// Linked list head and tail (dummy nodes for easier implementation)
	Node* head;
	Node* tail;
	
	// Hash and equality functors
	Hash hash_func;
	Equal equal_func;
	
	// Load factor management
	static const double MAX_LOAD_FACTOR;
	
	// Helper functions
	size_t get_hash(const Key& key) const {
		return hash_func(key) % table_size;
	}
	
	void rehash() {
		size_t new_size = table_size * 2;
		Node** new_table = new Node*[new_size];
		for (size_t i = 0; i < new_size; ++i) {
			new_table[i] = nullptr;
		}
		
		// Rehash all nodes
		Node* current = head->link_next;
		while (current != tail) {
			size_t new_hash = hash_func(current->get_data()->first) % new_size;
			current->next = new_table[new_hash];
			new_table[new_hash] = current;
			current = current->link_next;
		}
		
		delete[] table;
		table = new_table;
		table_size = new_size;
	}
	
	void init() {
		table_size = 16; // initial size
		element_count = 0;
		table = new Node*[table_size];
		for (size_t i = 0; i < table_size; ++i) {
			table[i] = nullptr;
		}
		
		// Initialize dummy head and tail for linked list
		head = new Node();
		tail = new Node();
		head->link_next = tail;
		tail->link_prev = head;
	}
	
	void clear_all() {
		// Clear hash table
		for (size_t i = 0; i < table_size; ++i) {
			Node* current = table[i];
			while (current != nullptr) {
				Node* next = current->next;
				delete current;
				current = next;
			}
		}
		delete[] table;
		
		// Clear linked list (excluding dummy nodes which are already deleted)
		delete head;
		delete tail;
	}
 
public:
	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = linked_hashmap.begin(); --it;
	 *       or it = linked_hashmap.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		Node* node_ptr;
		const linked_hashmap* container;
		
		friend class linked_hashmap;
		friend class const_iterator;
		
		iterator(Node* ptr, const linked_hashmap* cont) : node_ptr(ptr), container(cont) {}
		
	public:
		// The following code is written for the C++ type_traits library.
		// Type traits is a C++ feature for describing certain properties of a type.
		// For instance, for an iterator, iterator::value_type is the type that the 
		// iterator points to. 
		// STL algorithms and containers may use these type_traits (e.g. the following 
		// typedef) to work properly. 
		// See these websites for more information:
		// https://en.cppreference.com/w/cpp/header/type_traits
		// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
		// About iterator_category: https://en.cppreference.com/w/cpp/iterator
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::bidirectional_iterator_tag;

		iterator() : node_ptr(nullptr), container(nullptr) {}
		
		iterator(const iterator &other) : node_ptr(other.node_ptr), container(other.container) {}
		
		/**
		 * iter++
		 */
		iterator operator++(int) {
			if (node_ptr == nullptr || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			iterator temp = *this;
			node_ptr = node_ptr->link_next;
			return temp;
		}
		
		/**
		 * ++iter
		 */
		iterator & operator++() {
			if (node_ptr == nullptr || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			node_ptr = node_ptr->link_next;
			return *this;
		}
		
		/**
		 * iter--
		 */
		iterator operator--(int) {
			if (node_ptr == nullptr || node_ptr == container->head->link_next) {
				throw invalid_iterator();
			}
			iterator temp = *this;
			node_ptr = node_ptr->link_prev;
			return temp;
		}
		
		/**
		 * --iter
		 */
		iterator & operator--() {
			if (node_ptr == nullptr || node_ptr == container->head->link_next) {
				throw invalid_iterator();
			}
			node_ptr = node_ptr->link_prev;
			return *this;
		}
		
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		value_type & operator*() const {
			if (node_ptr == nullptr || node_ptr->is_dummy) {
				throw invalid_iterator();
			}
			return *(node_ptr->get_data());
		}
		
		bool operator==(const iterator &rhs) const {
			return node_ptr == rhs.node_ptr && container == rhs.container;
		}
		
		bool operator==(const const_iterator &rhs) const {
			return node_ptr == rhs.node_ptr && container == rhs.container;
		}
		
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
			return !(*this == rhs);
		}
		
		bool operator!=(const const_iterator &rhs) const {
			return !(*this == rhs);
		}

		/**
		 * for the support of it->first. 
		 * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
		 */
		value_type* operator->() const noexcept {
			if (node_ptr == nullptr || node_ptr->is_dummy) {
				throw invalid_iterator();
			}
			return node_ptr->get_data();
		}
	};
 
	class const_iterator {
	private:
		Node* node_ptr;
		const linked_hashmap* container;
		
		friend class linked_hashmap;
		friend class iterator;
		
		const_iterator(Node* ptr, const linked_hashmap* cont) : node_ptr(ptr), container(cont) {}
		
	public:
		const_iterator() : node_ptr(nullptr), container(nullptr) {}
		
		const_iterator(const const_iterator &other) : node_ptr(other.node_ptr), container(other.container) {}
		
		const_iterator(const iterator &other) : node_ptr(other.node_ptr), container(other.container) {}
		
		/**
		 * iter++
		 */
		const_iterator operator++(int) {
			if (node_ptr == nullptr || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			const_iterator temp = *this;
			node_ptr = node_ptr->link_next;
			return temp;
		}
		
		/**
		 * ++iter
		 */
		const_iterator & operator++() {
			if (node_ptr == nullptr || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			node_ptr = node_ptr->link_next;
			return *this;
		}
		
		/**
		 * iter--
		 */
		const_iterator operator--(int) {
			if (node_ptr == nullptr || node_ptr == container->head->link_next) {
				throw invalid_iterator();
			}
			const_iterator temp = *this;
			node_ptr = node_ptr->link_prev;
			return temp;
		}
		
		/**
		 * --iter
		 */
		const_iterator & operator--() {
			if (node_ptr == nullptr || node_ptr == container->head->link_next) {
				throw invalid_iterator();
			}
			node_ptr = node_ptr->link_prev;
			return *this;
		}
		
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		const value_type & operator*() const {
			if (node_ptr == nullptr || node_ptr->is_dummy) {
				throw invalid_iterator();
			}
			return *(node_ptr->get_data());
		}
		
		bool operator==(const iterator &rhs) const {
			return node_ptr == rhs.node_ptr && container == rhs.container;
		}
		
		bool operator==(const const_iterator &rhs) const {
			return node_ptr == rhs.node_ptr && container == rhs.container;
		}
		
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
			return !(*this == rhs);
		}
		
		bool operator!=(const const_iterator &rhs) const {
			return !(*this == rhs);
		}

		/**
		 * for the support of it->first. 
		 */
		const value_type* operator->() const noexcept {
			if (node_ptr == nullptr || node_ptr->is_dummy) {
				throw invalid_iterator();
			}
			return node_ptr->get_data();
		}
	};
 
	/**
	 * two constructors
	 */
	linked_hashmap() {
		init();
	}
	
	linked_hashmap(const linked_hashmap &other) {
		init();
		for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
			insert(*it);
		}
	}
 
	/**
	 * assignment operator
	 */
	linked_hashmap & operator=(const linked_hashmap &other) {
		if (this != &other) {
			clear_all();
			init();
			for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
				insert(*it);
			}
		}
		return *this;
	}
 
	/**
	 * Destructors
	 */
	~linked_hashmap() {
		clear_all();
	}
 
	/**
	 * access specified element with bounds checking
	 * Returns a reference to the mapped value of the element with key equivalent to key.
	 * If no such element exists, an exception of type `index_out_of_bound'
	 */
	T & at(const Key &key) {
		size_t hash_val = get_hash(key);
		Node* current = table[hash_val];
		while (current != nullptr) {
			if (equal_func(current->get_data()->first, key)) {
				return current->get_data()->second;
			}
			current = current->next;
		}
		throw index_out_of_bound();
	}
	
	const T & at(const Key &key) const {
		size_t hash_val = get_hash(key);
		Node* current = table[hash_val];
		while (current != nullptr) {
			if (equal_func(current->get_data()->first, key)) {
				return current->get_data()->second;
			}
			current = current->next;
		}
		throw index_out_of_bound();
	}
 
	/**
	 * access specified element 
	 * Returns a reference to the value that is mapped to a key equivalent to key,
	 *   performing an insertion if such key does not already exist.
	 */
	T & operator[](const Key &key) {
		size_t hash_val = get_hash(key);
		Node* current = table[hash_val];
		while (current != nullptr) {
			if (equal_func(current->get_data()->first, key)) {
				return current->get_data()->second;
			}
			current = current->next;
		}
		
		// Key not found, insert with default value
		value_type new_pair(key, T());
		pair<iterator, bool> result = insert(new_pair);
		return result.first->second;
	}
 
	/**
	 * behave like at() throw index_out_of_bound if such key does not exist.
	 */
	const T & operator[](const Key &key) const {
		return at(key);
	}
 
	/**
	 * return a iterator to the beginning
	 */
	iterator begin() {
		return iterator(head->link_next, this);
	}
	
	const_iterator cbegin() const {
		return const_iterator(head->link_next, this);
	}
 
	/**
	 * return a iterator to the end
	 * in fact, it returns past-the-end.
	 */
	iterator end() {
		return iterator(tail, this);
	}
	
	const_iterator cend() const {
		return const_iterator(tail, this);
	}
 
	/**
	 * checks whether the container is empty
	 * return true if empty, otherwise false.
	 */
	bool empty() const {
		return element_count == 0;
	}
 
	/**
	 * returns the number of elements.
	 */
	size_t size() const {
		return element_count;
	}
 
	/**
	 * clears the contents
	 */
	void clear() {
		clear_all();
		init();
	}
 
	/**
	 * insert an element.
	 * return a pair, the first of the pair is
	 *   the iterator to the new element (or the element that prevented the insertion), 
	 *   the second one is true if insert successfully, or false.
	 */
	pair<iterator, bool> insert(const value_type &value) {
		size_t hash_val = get_hash(value.first);
		Node* current = table[hash_val];
		
		// Check if key already exists
		while (current != nullptr) {
			if (equal_func(current->get_data()->first, value.first)) {
				return pair<iterator, bool>(iterator(current, this), false);
			}
			current = current->next;
		}
		
		// Create new node
		Node* new_node = new Node(value);
		
		// Insert into hash table
		new_node->next = table[hash_val];
		table[hash_val] = new_node;
		
		// Insert into linked list at the end
		new_node->link_prev = tail->link_prev;
		new_node->link_next = tail;
		tail->link_prev->link_next = new_node;
		tail->link_prev = new_node;
		
		element_count++;
		
		// Check if rehashing is needed
		if (static_cast<double>(element_count) / table_size > MAX_LOAD_FACTOR) {
			rehash();
		}
		
		return pair<iterator, bool>(iterator(new_node, this), true);
	}
 
	/**
	 * erase the element at pos.
	 *
	 * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
	 */
	void erase(iterator pos) {
		if (pos.node_ptr == nullptr || pos.node_ptr == tail || pos.container != this) {
			throw invalid_iterator();
		}
		
		Node* to_erase = pos.node_ptr;
		
		// Remove from hash table
		size_t hash_val = get_hash(to_erase->get_data()->first);
		Node* current = table[hash_val];
		Node* prev = nullptr;
		
		while (current != nullptr) {
			if (current == to_erase) {
				if (prev == nullptr) {
					table[hash_val] = current->next;
				} else {
					prev->next = current->next;
				}
				break;
			}
			prev = current;
			current = current->next;
		}
		
		// Remove from linked list
		to_erase->link_prev->link_next = to_erase->link_next;
		to_erase->link_next->link_prev = to_erase->link_prev;
		
		delete to_erase;
		element_count--;
	}
 
	/**
	 * Returns the number of elements with key 
	 *   that compares equivalent to the specified argument,
	 *   which is either 1 or 0 
	 *     since this container does not allow duplicates.
	 */
	size_t count(const Key &key) const {
		size_t hash_val = get_hash(key);
		Node* current = table[hash_val];
		while (current != nullptr) {
			if (equal_func(current->get_data()->first, key)) {
				return 1;
			}
			current = current->next;
		}
		return 0;
	}
 
	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is returned.
	 */
	iterator find(const Key &key) {
		size_t hash_val = get_hash(key);
		Node* current = table[hash_val];
		while (current != nullptr) {
			if (equal_func(current->get_data()->first, key)) {
				return iterator(current, this);
			}
			current = current->next;
		}
		return end();
	}
	
	const_iterator find(const Key &key) const {
		size_t hash_val = get_hash(key);
		Node* current = table[hash_val];
		while (current != nullptr) {
			if (equal_func(current->get_data()->first, key)) {
				return const_iterator(current, this);
			}
			current = current->next;
		}
		return cend();
	}
};
 
// Define static member
template<class Key, class T, class Hash, class Equal>
const double linked_hashmap<Key, T, Hash, Equal>::MAX_LOAD_FACTOR = 0.75;
 
}
 
#endif
