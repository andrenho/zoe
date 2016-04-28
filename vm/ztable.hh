#ifndef VM_ZTABLE_H_
#define VM_ZTABLE_H_

#include <unordered_map>
using namespace std;

#include "vm/zvalue.hh"
#include "vm/opcode.hh"  // TODO - for TableConfig

struct ZTableHash {
    
    size_t operator()(shared_ptr<ZValue> const& k) const {
        return k->Hash();
    }

    bool operator()(shared_ptr<ZValue> const& a, shared_ptr<ZValue> const& b) const {
        return a->OpEq(b);
    }

};


typedef unordered_map<shared_ptr<ZValue>, shared_ptr<ZValue>, ZTableHash, ZTableHash> ZTableHashMap;

class ZTable : public ZValue {
public:
    explicit ZTable(TableConfig tc) : ZValue(StaticType()), _config(tc) {}
    template<typename Iter> ZTable(Iter const& _begin, Iter const& _end, TableConfig tc) : ZTable(tc) {{{
        for(auto t = _begin; t != _end;) {   // copy iterator because it is const
            auto key = *t++;
            _items[key] = *t++;
        }
    }}}

    string Inspect() const override;

    bool OpEq(shared_ptr<ZValue> other) const override;
    void OpSet(shared_ptr<ZValue> key, shared_ptr<ZValue> value, TableConfig tc) override;
    shared_ptr<ZValue> OpGet(shared_ptr<ZValue> key) const override;

    static ZType StaticType() { return TABLE; }
    ZTableHashMap const& Value() const { return _items; }

private:
    static string InspectProperties(TableConfig tc);

    ZTableHashMap _items = {};
    TableConfig _config;
};

#endif

// vim: ts=4:sw=4:sts=4:expandtab:foldmethod=marker:syntax=cpp
