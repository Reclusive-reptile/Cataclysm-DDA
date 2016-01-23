#ifndef INVENTORY_H
#define INVENTORY_H

#include "item.h"
#include "enums.h"

#include <list>
#include <string>
#include <utility>
#include <vector>
#include <functional>

class map;
class npc;

typedef std::list< std::list<item> > invstack;
typedef std::vector< std::list<item>* > invslice;
typedef std::vector< const std::list<item>* > const_invslice;
typedef std::vector< std::pair<std::list<item>*, int> > indexed_invslice;
typedef std::function<bool(const item &)> item_filter;

class salvage_actor;

/**
 * Wrapper to handled a set of valid "inventory" letters. "inventory" can be any set of
 * objects that the player can access via a single character (e.g. bionics).
 * The class is (currently) derived from std::string for compatibility and because it's
 * simpler. But it may be changed to derive from `std::set<long>` or similar to get the full
 * range of possible characters.
 */
class invlet_wrapper : private std::string {
private:

public:
    invlet_wrapper( const char *chars ) : std::string( chars ) { }

    bool valid( long invlet ) const;
    std::string get_allowed_chars() const { return *this; }

    using std::string::begin;
    using std::string::end;
    using std::string::rbegin;
    using std::string::rend;
    using std::string::size;
    using std::string::length;
};

const extern invlet_wrapper inv_chars;

class inventory
{
    public:
        invslice slice();
        const_invslice const_slice() const;
        const std::list<item> &const_stack(int i) const;
        size_t size() const;
        int num_items() const;
        bool is_sorted() const;

        inventory();
        inventory(inventory &&) = default;
        inventory(const inventory &) = default;
        inventory &operator=(inventory &&) = default;
        inventory &operator=(const inventory &) = default;

        inventory &operator+= (const inventory &rhs);
        inventory &operator+= (const item &rhs);
        inventory &operator+= (const std::list<item> &rhs);
        inventory &operator+= (const std::vector<item> &rhs);
        inventory  operator+  (const inventory &rhs);
        inventory  operator+  (const item &rhs);
        inventory  operator+  (const std::list<item> &rhs);

        static bool has_activation(const item &it, const player &u);
        static bool has_capacity_for_liquid(const item &it, const item &liquid);

        indexed_invslice slice_filter();  // unfiltered, but useful for a consistent interface.
        indexed_invslice slice_filter_by_activation(const player &u);
        indexed_invslice slice_filter_by_capacity_for_liquid(const item &liquid);
        indexed_invslice slice_filter_by_flag(const std::string flag);
        indexed_invslice slice_filter_by_salvageability(const salvage_actor &actor);

        void unsort(); // flags the inventory as unsorted
        void sort();
        void clear();
        void add_stack(std::list<item> newits);
        void clone_stack(const std::list<item> &rhs);
        void push_back(std::list<item> newits);
        // returns a reference to the added item
        item &add_item (item newit, bool keep_invlet = false, bool assign_invlet = true);
        void add_item_keep_invlet(item newit);
        void push_back(item newit);

        /* Check all items for proper stacking, rearranging as needed
         * game pointer is not necessary, but if supplied, will ensure no overlap with
         * the player's worn items / weapon
         */
        void restack(player *p = NULL);

        void form_from_map( const tripoint &origin, int distance, bool assign_invlet = true );

        /**
         * Remove a specific item from the inventory. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param it A pointer to the item to be removed. The item *must* exists
         * in this inventory.
         * @return A copy of the removed item.
         */
        item remove_item(const item *it);
        item remove_item(int position);
        /**
         * Randomly select items until the volume quota is filled.
         */
        std::list<item> remove_randomly_by_volume(int volume);
        std::list<item> reduce_stack(int position, int quantity);
        std::list<item> reduce_stack(const itype_id &type, int quantity);

        // amount of -1 removes the entire stack.
        template<typename Locator> std::list<item> reduce_stack(const Locator &type, int amount);

        const item &find_item(int position) const;
        item &find_item(int position);
        item &item_by_type(itype_id type);
        item &item_or_container(itype_id type); // returns an item, or a container of it

        /**
         * Returns the item position of the stack that contains the given item (compared by
         * pointers). Returns INT_MIN if the item is not found.
         * Note that this may lose some information, for example the returned position is the
         * same when the given item points to the container and when it points to the item inside
         * the container. All items that are part of the same stack have the same item position.
         */
        int position_by_item( const item *it ) const;
        int position_by_type(itype_id type);
        /** Return the item position of the item with given invlet, return INT_MIN if
         * the inventory does not have such an item with that invlet. Don't use this on npcs inventory. */
        int invlet_to_position(char invlet) const;

        std::vector<std::pair<item *, int> > all_items_by_type(itype_id type);

        // Below, "amount" refers to quantity
        //        "charges" refers to charges
        int  amount_of (itype_id it) const;
        int  amount_of (itype_id it, bool used_as_tool) const;
        long charges_of(itype_id it) const;

        std::list<item> use_amount (itype_id it, int quantity);
        std::list<item> use_charges(itype_id it, long quantity);

        bool has_amount (itype_id it, int quantity) const;
        bool has_amount (itype_id it, int quantity, bool used_as_tool) const;
        bool has_tools (itype_id it, int quantity) const;
        bool has_components (itype_id it, int quantity) const;
        bool has_charges(itype_id it, long quantity) const;
        /**
         * Check whether a specific item is in this inventory.
         * The item is compared by pointer.
         * @param it A pointer to the item to be looked for.
         */
        bool has_item(const item *it) const;
        bool has_items_with_quality(std::string id, int level, int amount) const;

        // Returns max required quality in player's items, INT_MIN if player has no such items
        int max_quality( const std::string &quality_id ) const;

        static int num_items_at_position( int position );

        int leak_level(std::string flag) const; // level of leaked bad stuff from items

        // NPC/AI functions
        int worst_item_value(npc *p) const;
        bool has_enough_painkiller(int pain) const;
        item *most_appropriate_painkiller(int pain);
        item *best_for_melee( player &p, double &best );
        item *most_loaded_gun();

        void rust_iron_items();

        int weight() const;
        int volume() const;

        void dump(std::vector<item *> &dest); // dumps contents into dest (does not delete contents)

        // vector rather than list because it's NOT an item stack
        std::vector<item *> active_items();

        void json_load_invcache(JsonIn &jsin);
        void json_load_items(JsonIn &jsin);

        void json_save_invcache(JsonOut &jsout) const;
        void json_save_items(JsonOut &jsout) const;

        item nullitem;
        std::list<item> nullstack;

        // Assigns an invlet if any remain.  If none do, will assign ` if force is
        // true, empty (invlet = 0) otherwise.
        void assign_empty_invlet(item &it, bool force = false);

        std::set<char> allocated_invlets() const;

        template<typename T>
        indexed_invslice slice_filter_by( T filter )
        {
            int i = 0;
            indexed_invslice stacks;
            for( auto &elem : items ) {
                if( filter( elem.front() ) ) {
                    stacks.push_back( std::make_pair( &elem, i ) );
                }
                ++i;
            }
            return stacks;
        }

        /** Traverses each item in the inventory using a visitor function
         * @return Similar to item::visit_items returns only VisitResponse::Next or VisitResponse::Abort
         * @see item::visit_items
         **/
        VisitResponse visit_items( const std::function<VisitResponse(item *, item *)>& func );
        VisitResponse visit_items( const std::function<VisitResponse(const item *, const item *)>& func ) const;

        /** Returns true if any item (including those within a container) matches the filter */
        bool has_item_with( const std::function<bool(const item&)>& filter ) const;

        /** Returns all items matching the filter including any within containers */
        std::vector<item *> items_with( const std::function<bool(const item&)>& filter );
        std::vector<const item *> items_with( const std::function<bool(const item&)>& filter ) const;

        template<typename T>
        std::list<item> remove_items_with( T filter )
        {
            std::list<item> result;
            for( auto items_it = items.begin(); items_it != items.end(); ) {
                auto &stack = *items_it;
                for( auto stack_it = stack.begin(); stack_it != stack.end(); ) {
                    if( filter( *stack_it ) ) {
                        result.push_back( std::move( *stack_it ) );
                        stack_it = stack.erase( stack_it );
                        if( stack_it == stack.begin() && !stack.empty() ) {
                            // preserve the invlet when removing the first item of a stack
                            stack_it->invlet = result.back().invlet;
                        }
                    } else {
                        result.splice( result.begin(), stack_it->remove_items_with( filter ) );
                        ++stack_it;
                    }
                }
                if( stack.empty() ) {
                    items_it = items.erase( items_it );
                } else {
                    ++items_it;
                }
            }
            return result;
        }
    private:
        // For each item ID, store a set of "favorite" inventory letters.
        std::map<std::string, std::vector<char> > invlet_cache;
        void update_cache_with_item(item &newit);
        char find_usable_cached_invlet(const std::string &item_type);

        // Often items can be located using typeid, position, or invlet.  To reduce code duplication,
        // we back those functions with a single internal function templated on the type of Locator.
        template<typename Locator> item remove_item_internal(const Locator &locator);
        template<typename Locator> std::list<item> reduce_stack_internal(const Locator &type, int amount);

        invstack items;
        bool sorted;
};

#endif
