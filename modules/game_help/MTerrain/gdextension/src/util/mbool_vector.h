#ifndef __BOOLVEC__
#define __BOOLVEC__

#include "core/templates/vector.h"

class MBoolVector{
    private:
    int64_t _size = 0;
    Vector<uint8_t> _data;

    public:
    _FORCE_INLINE_ void resize(const int64_t size){
        _size = size;
        _data.resize((size/8) + (int64_t)(size%8!=0));
    }

    _FORCE_INLINE_ int64_t size() const{
        return _size;
    }

    _FORCE_INLINE_ void set(const int64_t index,const bool value){
        ERR_FAIL_INDEX(index,_size);
        int64_t dindex = index/8;
        int bit = index%8;
        if(value){
            _data.set(dindex,_data[dindex] | (1 << bit));
        } else {
            _data.set(dindex,_data[dindex] & ~(1 << bit));
        }
    }

    _FORCE_INLINE_ bool get(const int64_t index) const{
        ERR_FAIL_INDEX_V(index,_size,false);
        int64_t dindex = index/8;
        int bit = index%8;
        return _data[dindex] & 1 << bit;
    }

    _FORCE_INLINE_ void push_back(bool value){
        int64_t dindex = _size/8;
        int bit = _size%8;
        ++_size;
        _data.resize((_size/8) + (int64_t)(_size%8!=0));
        if(value){
            _data.set(dindex,_data[dindex] | (1 << bit));
        } else {
            _data.set(dindex,_data[dindex] & ~(1 << bit));
        }
    }

    _FORCE_INLINE_ void fill_false(){
        memset(_data.ptrw(),0,_data.size());
    }

    _FORCE_INLINE_ void fill_true(){
        memset(_data.ptrw(),0xFF,_data.size());
    }

    _FORCE_INLINE_ bool has_any_true() const {
        int64_t s = _data.size() - 1;
        for(int64_t i=0; i < s; ++i){
            if(_data[i]!=0){
                return true;
            }
        }
        // remaining
        uint8_t val = _data[s];
        uint8_t r = _size%8;
        uint8_t mask = (r == 0) ? 0 : (0xFF << (8 - r));
        return (mask & val) != 0;
    }

    _FORCE_INLINE_ bool has_any_false() const {
        int64_t s = _data.size() - 1;
        for(int64_t i=0; i < s; ++i){
            if(_data[i]!=0xFF){
                return true;
            }
        }
        // remaining
        uint8_t val = _data[s];
        uint8_t r = _size%8;
        uint8_t mask = (r == 0) ? 0 : (0xFF << (8 - r));
        return (mask & val) != mask;
    }

    _FORCE_INLINE_ bool operator[](const int64_t index) const{
        return get(index);
    }
};
#endif