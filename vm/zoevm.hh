#ifndef VM_ZOEVM_H_
#define VM_ZOEVM_H_

#include <cstdint>
#include <memory>
#include <vector>
using namespace std;

#include "vm/zvalue.hh"
#include "vm/ztable.hh"

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
    void               Pop(uint16_t n);
    void               Remove(ssize_t pos);
    ZValue const*      GetPtr(ssize_t pos=-1) const;
    shared_ptr<ZValue> GetCopy(ssize_t pos=-1) const;
    // {{{ stack templates: Pop<T>(), GetPtr<T>(), GetCopy<T>(), T CopyCppValue()
    template<typename T> shared_ptr<T> Pop() {
        auto ptr = Pop();
        ValidateType<T>(ptr->Type());
        return static_pointer_cast<T>(ptr);
    }

    template<typename T> T const* GetPtr(ssize_t pos=-1) const {
        ZValue const* ptr = GetPtr(pos);
        ValidateType<T>(ptr->Type());
        return static_cast<T const*>(ptr);
    }

    template<typename T> shared_ptr<T> GetCopy(ssize_t pos=-1) const {
        auto ptr = GetCopy(pos);
        ValidateType<T>(ptr->Type());
        return static_pointer_cast<T>(ptr);
    }

    template<typename T> T CopyCppValue(ssize_t pos=-1) const {
        auto ptr = GetPtr<typename cpp_type<T>::type>(pos);
        return static_cast<T>(ptr->Value());
    }

private:
    template<typename T> void ValidateType(ZType type) const {
        if(type != T::StaticType()) {
            throw zoe_runtime_error("Invalid type: expected " + Typename(T::StaticType()) + ", found " + Typename(type));
        }
    }
public:
    //}}}
    
    // 
    // code execution
    //
    void ExecuteBytecode(vector<uint8_t> const& bytecode);

    // 
    // debugging
    //
    bool Tracer = false;

private:
    void CreateVariables(uint16_t n);

    vector<shared_ptr<ZValue>> _stack = {};
    vector<shared_ptr<ZValue>> _vars = {};
    vector<uint32_t> _scopes = { 0 };
    vector<uint64_t> _call_stack = {};
};

#endif

// vim: ts=4:sw=4:sts=4:expandtab:foldmethod=marker:syntax=cpp
