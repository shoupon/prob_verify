/****************************************************************************
  FileName     [ myHash.h ]
  PackageName  [ util ]
  Synopsis     [ Define Hash and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_H
#define MY_HASH_H

//#define HASH_DEBUG
#include <vector>

using namespace std;

//--------------------
// Define Hash classes
//--------------------
// To use Hash ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
/*
class BddHashKey
{
public:
   BddHashKey(size_t l, size_t r, unsigned i):_left(l), _right(r), _level(i) {}
   size_t operator() () const { return ((l>>2)+(r>>2)); }
   bool operator == (const HashKey& k) 
   {
      return ( (_left==k._left) && (_right==k._right) && (_level==k._level ) );
   }
 
private:
   size_t _left ;
   size_t _right ;
   unsigned _level ;
};
*/

template <class HashKey, class HashData>
class Hash
{
typedef pair<HashKey, HashData> HashNode;

public:
   Hash() : _numBuckets(0), _buckets(0) {}
   Hash(size_t b) { init(b); }
   ~Hash() { reset(); }

   // TODO: implement the Hash<HashKey, HashData>::iterator
   // o An iterator should be able to go through all the valid HashNodes
   //   in the Hash
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   // (_bId, _bnId) range from (0, 0) to (_numBuckets, 0)
   //
   class iterator
   {
      friend class Hash<HashKey, HashData>;

   public:
      iterator():_ptr(0), _key(0), _pos(0) {}
      iterator(Hash* p, size_t k, size_t pos)
         :_ptr(p),_key(k),_pos(pos){}
      iterator(const iterator& i):_ptr(i._ptr), _key(i._key), _pos(i._pos) {}
      ~iterator() {}

      const HashNode& operator * ()const{ return _ptr->_buckets[_key][_pos];}
      HashNode& operator * () { return _ptr->_buckets[_key][_pos] ; }
      iterator& operator ++ () 
      { 
         if( ++_pos == _ptr->_buckets[_key].size() ) {
            _pos = 0 ;
            while(++_key < _ptr->_numBuckets) 
               if( !_ptr->_buckets[_key].empty() )
                  break ;
         }
         return *this ; 
      }
      iterator operator ++ (int)
      {
         iterator temp(*this) ;
         ++(*this) ;
         return temp ;
      }
      iterator& operator -- ()
      {
         if( _pos == 0 ) 
            do {
               --_key ;
               if( !_ptr->_buckets[_key].empty() ) {
                  _pos = _ptr->_buckets[_key].size() - 1 ;
                  break ;
               }
            } while( _key != 0 ) ;
         else
            --_pos ;
         return *this ;
      }
      iterator operator -- (int)
      {
         iterator temp(*this) ;
         --(*this) ;
         return temp ;
      }
      iterator& operator = (const iterator& i) 
      {
         _key = i._key ; 
         _pos = i._pos ;
         _ptr = i._ptr ;
         return *this ;
      }

     bool operator != (const iterator& i) const
     {
        if( i._key != _key )
           return true ;
        else if( i._pos != _pos )
           return true ;
        return false ;
     }
     bool operator == (const iterator& i) const { return !((*this) != i) ; }
            
   private:
      Hash* _ptr ;
      size_t _key ;
      size_t _pos ;
   };

   // Point to the first valid data
   iterator begin() 
   { 
      size_t key = 0; 
      while( _buckets[key].empty() )
         ++key ;
      return iterator(this, key, 0); 
   }
   // Pass the end
   iterator end() 
   { 
      return iterator( this, _numBuckets, 0 ) ;
   }
   // return true if no valid data
   bool empty() const 
   { 
      for( size_t i = 0 ; i < _numBuckets ; ++i ) 
         if( _buckets[i].size() )
            return false ;
      return true; 
   }

   // number of valid data
    size_t size() 
    {
#ifdef HASH_DEBUG
        _hist.clear();
        _hist.reserve(_numBuckets);
#endif
        size_t s = 0;
        for( size_t i = 0 ; i < _numBuckets ; ++i ) {
            s += _buckets[i].size() ;
#ifdef HASH_DEBUG
            _hist.push_back(_buckets[i].size());
#endif
        }
        return s;
    }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

   void init(size_t b) 
   { 
      _numBuckets = b ;  
      _buckets = new vector<HashNode>[_numBuckets] ;
   }

   void reset() 
   { 
      for( size_t key = 0 ; key < _numBuckets ; ++key )
         _buckets[key].clear() ;
      delete[] _buckets ;
      _numBuckets = 0 ;
   }

    // check if k is in the hash...
    // if yes, update n and return true;
    // else return false;
    bool check(const HashKey& k, HashData& n) 
    { 
        size_t key = bucketNum(k) ;
        for( size_t i = 0 ; i < _buckets[key].size() ; ++i )
            if( _buckets[key][i].first == k ) {
                n = _buckets[key][i].second ;
                return true ;
            }
        return false; 
    }

    // return true if inserted successfully (i.e. k is not in the hash)
    // return false is k is already in the hash ==> will not insert
    bool insert(const HashKey& k, const HashData& d) 
    { 
        size_t key = bucketNum(k) ;
        if( find(k)!=end() )
            return false ;
        _buckets[key].push_back(make_pair(k,d)) ;
        return true; 
    }

    // return true if inserted successfully (i.e. k is not in the hash)
    // return false is k is already in the hash ==> still do the insertion
    bool replaceInsert(const HashKey& k, const HashData& d) 
    { 
        iterator it = find(k) ;
        if( it == end() ){
            insert(k, d) ;
            return true ;
        }
        else {
            (*it).second = d ;
            return false ;
        }
    }

    // Need to be sure that k is not in the hash
    void forceInsert(const HashKey& k, const HashData& d) 
    { 
        replaceInsert(k, d) ;
    }

    iterator find(const HashKey& k)
    {
        size_t key = bucketNum(k) ;
        for( size_t i = 0; i < _buckets[key].size() ; ++i )
            if( _buckets[key][i].first == k )
                return iterator(this, key, i) ;
        return end();
    }

private:
    size_t                   _numBuckets;
    vector<HashNode>*        _buckets;
    
#ifdef HASH_DEBUG
    vector<int>              _hist;
#endif

    size_t bucketNum(const HashKey& k) const {
        return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
/*
class BddCacheKey
{
public:
   BddCacheKey(size_t a, size_t b, size_t c):_i(a), _t(b), _e(c) {}
   
   size_t operator() () const { return ( (_i>>2)+(_t>>2)+(_e>>2) ); }
  
   bool operator == (const CacheKey& k ) const 
   { 
      return ( (_i==k._i)&&(_t==k._t)&&(_e==k._e) ) ;
   }
      
private:
   size_t _i ;
   size_t _t ;
   size_t _e ;
};
*/ 

template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
    Cache() : _size(0), _cache(0) {}
    Cache(size_t s) { init(s); }
    ~Cache() { reset(); }

    // NO NEED to implement Cache::iterator class

    // Initialize _cache with size s
    void init(size_t s) { _size = s ; _cache = new CacheNode[_size] ;  }
    void reset() { delete[] _cache ; _size = 0 ; }

    size_t size() const { return _size; }

    CacheNode& operator [] (size_t i) { return _cache[i]; }
    const CacheNode& operator [](size_t i) const { return _cache[i]; }

    // return false if cache miss
    bool read(const CacheKey& k, CacheData& d) const 
    { 
        if( _cache[k()%_size].first == k )
         d = _cache[k()%_size].second ;
        else
         return false ;
        return true ;
    }

    // If k is already in the Cache, overwrite the CacheData
    void write(const CacheKey& k, const CacheData& d) 
    { 
        _cache[k()%_size].first = k ;
        _cache[k()%_size].second = d ;
    }

private:
    // Do not add any extra data member
    size_t         _size;
    CacheNode*     _cache;
};


#endif // MY_HASH_H
