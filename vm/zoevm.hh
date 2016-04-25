#ifndef VM_ZOEVM_H_
#define VM_ZOEVM_H_

#include <cstdint>
#include <memory>
#include <vector>
using namespace std;

#include "vm/zvalue.hh"

class ZoeVM {
public:
    ZoeVM();

    // 
    // stack management
    //
    ssize_t            StackAbs(ssize_t pos) const;
    ssize_t            StackSize() const;
    ZValue const&      Push(shared_ptr<ZValue> value);
    ZType              GetType(ssize_t pos=-1) const;
    shared_ptr<ZValue> Pop();
    ZValue const*      GetPtr(ssize_t pos=-1) const;
    shared_ptr<ZValue> GetCopy(ssize_t pos=-1) const;
    // {{{ stack templates: Pop<T>(), GetPtr<T>(), GetCopy<T>()
    template<typename T> shared_ptr<T> Pop() {
        auto ptr = dynamic_pointer_cast<T>(Pop());
        ValidateType<T>(ptr->Type());
        return ptr;
    }

    template<typename T> T const* GetPtr(ssize_t pos=-1) const {
        auto ptr = GetCopy(pos);
        ValidateType<T>(ptr->Type());
        return dynamic_cast<T const*>(ptr.get());
    }

    template<typename T> shared_ptr<T> GetCopy(ssize_t pos=-1) const {
        auto ptr = dynamic_pointer_cast<T>(GetCopy(pos));
        ValidateType<T>(ptr->Type());
        return ptr;
    }

private:
    template<typename T> void ValidateType(ZType type) const {
        if(type != T::StaticType()) {
            throw "Invalid type: expected " + Typename(T::StaticType()) + ", found " + Typename(type);
        }
    }
public:
    //}}}
    
    // 
    // code execution
    //
    void ExecuteBytecode(vector<uint8_t> const& bytecode);

private:
    vector<shared_ptr<ZValue>> _stack = {};
};

#endif

// vim: ts=4:sw=4:sts=4:expandtab:foldmethod=marker:syntax=cpp