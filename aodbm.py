''' 
    aodbm - Append Only Database Manager
    Copyright (C) 2011 Daniel Waterworth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

import ctypes

class Data(ctypes.Structure):
    _fields_ = [("dat", ctypes.c_char_p),
                ("sz", ctypes.c_size_t)]

data_ptr = ctypes.POINTER(Data)

class Record(ctypes.Structure):
    _fields_ = [("key", data_ptr),
                ("val", data_ptr)]

def str_to_data(st):
    return Data(st, len(st))

def data_to_str(dat):
    return ctypes.string_at(dat.dat, dat.sz)

aodbm_lib = ctypes.CDLL("./libaodbm.so")

aodbm_lib.aodbm_open.argtypes = [ctypes.c_char_p, ctypes.c_int]
aodbm_lib.aodbm_open.restype = ctypes.c_void_p

aodbm_lib.aodbm_close.argtypes = [ctypes.c_void_p]
aodbm_lib.aodbm_close.restype = None

aodbm_lib.aodbm_current.argtypes = [ctypes.c_void_p]
aodbm_lib.aodbm_current.restype = ctypes.c_uint64

aodbm_lib.aodbm_commit.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
aodbm_lib.aodbm_commit.restype = ctypes.c_bool

aodbm_lib.aodbm_has.argtypes = [ctypes.c_void_p, ctypes.c_uint64, data_ptr]
aodbm_lib.aodbm_has.restype = ctypes.c_bool

aodbm_lib.aodbm_get.argtypes = [ctypes.c_void_p, ctypes.c_uint64, data_ptr]
aodbm_lib.aodbm_get.restype = data_ptr

aodbm_lib.aodbm_set.argtypes = [ctypes.c_void_p, ctypes.c_uint64, data_ptr, data_ptr]
aodbm_lib.aodbm_set.restype = ctypes.c_uint64

aodbm_lib.aodbm_del.argtypes = [ctypes.c_void_p, ctypes.c_uint64, data_ptr]
aodbm_lib.aodbm_del.restype = ctypes.c_uint64

aodbm_lib.aodbm_is_based_on.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.c_uint64]
aodbm_lib.aodbm_is_based_on.restype = ctypes.c_bool

aodbm_lib.aodbm_previous_version.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
aodbm_lib.aodbm_previous_version.restype = ctypes.c_uint64

aodbm_lib.aodbm_new_iterator.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
aodbm_lib.aodbm_new_iterator.restype = ctypes.c_void_p

aodbm_lib.aodbm_iterate_from.argtypes = [ctypes.c_void_p, ctypes.c_uint64, data_ptr]
aodbm_lib.aodbm_iterate_from.restype = ctypes.c_void_p

aodbm_lib.aodbm_iterator_goto.argtypes = [ctypes.c_void_p, ctypes.c_void_p, data_ptr]
aodbm_lib.aodbm_iterator_goto.restype = None

aodbm_lib.aodbm_iterator_next.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
aodbm_lib.aodbm_iterator_next.restype = Record

aodbm_lib.aodbm_free_iterator.argtypes = [ctypes.c_void_p]
aodbm_lib.aodbm_free_iterator.restype = None

aodbm_lib.aodbm_free_data.argtypes = [ctypes.c_void_p]
aodbm_lib.aodbm_free_data.restype = None

class VersionIterator(object):
    def __init__(self, version, it=None):
        self.version = version
        if it:
            self.it = it
        else:
            self.it = aodbm_lib.aodbm_new_iterator(version.db.db, version.version)
    
    def __del__(self):
        aodbm_lib.aodbm_free_iterator(self.it)
    
    def __iter__(self):
        return self
    
    def next(self):
        rec = aodbm_lib.aodbm_iterator_next(self.version.db.db, self.it)
        if rec.key:
            key = data_to_str(rec.key.contents)
            val = data_to_str(rec.val.contents)
            aodbm_lib.aodbm_free_data(rec.key)
            aodbm_lib.aodbm_free_data(rec.val)
            return key, val
        raise StopIteration()
    
    def goto(self, key):
        self.it = aodbm_lib.aodbm_iterator_goto(self.version.db.db, self.it, str_to_data(key))

class Version(object):
    '''Represents a version of the database'''
    def __init__(self, db, version):
        '''Don't use this method directly'''
        self.db = db
        self.version = version
    
    def has(self, key):
        '''Doesn't this version have key?'''
        return aodbm_lib.aodbm_has(self.db.db, self.version, str_to_data(key))
    
    def __getitem__(self, key):
        '''Queries the version for a key'''
        ptr = aodbm_lib.aodbm_get(self.db.db, self.version, str_to_data(key))
        if ptr:
            out = data_to_str(ptr.contents)
            aodbm_lib.aodbm_free_data(ptr)
            return out
        raise KeyError()
    
    def __setitem__(self, key, val):
        '''Set a record, changing the version in place'''
        key = str_to_data(key)
        val = str_to_data(val)
        self.version = aodbm_lib.aodbm_set(self.db.db, self.version, key, val)
    
    def __delitem__(self, key):
        '''Delete a key, changing the version in place'''
        key = str_to_data(key)
        self.version = aodbm_lib.aodbm_del(self.db.db, self.version, key)
    
    def del_key(self, key):
        '''Delete a key, returning the new database version object'''
        key = str_to_data(key)
        return Version(self.db, aodbm_lib.aodbm_del(self.db.db, self.version, key))
    
    def set_record(self, key, val):
        '''Set a record, returning the new database version object'''
        key = str_to_data(key)
        val = str_to_data(val)
        return Version(self.db, aodbm_lib.aodbm_set(self.db.db, self.version, key, val))
    
    def previous(self):
        '''Get the previous version object'''
        return Version(self.db, aodbm_lib.aodbm_previous_version(self.db.db, self.version))
    
    def is_based_on(self, other):
        '''Is this object based on other?'''
        assert self.db == other.db
        return aodbm_lib.aodbm_is_based_on(self.db.db, self.version, other.version)

    def __iter__(self):
        return VersionIterator(self)
    
    def iterate_from(self, key):
        return VersionIterator(self, aodbm_lib.aodbm_iterate_from(self.db.db, self.version, str_to_data(key)))

class AODBM(object):
    '''Represents a Database'''
    def __init__(self, filename, flags=0):
        '''Open a new database'''
        self.db = aodbm_lib.aodbm_open(filename, flags)
    
    def __del__(self):
        aodbm_lib.aodbm_close(self.db)
    
    def current_version(self):
        '''Get the current version object'''
        return Version(self, aodbm_lib.aodbm_current(self.db))
    
    def commit(self, version):
        '''Commits the version object to the database.'''
        assert self == version.db
        return aodbm_lib.aodbm_commit(self.db, version.version)
