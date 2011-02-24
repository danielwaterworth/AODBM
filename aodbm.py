import ctypes

class Data(ctypes.Structure):
    _fields_ = [("dat", ctypes.c_char_p),
                ("sz", ctypes.c_size_t)]

def str_to_data(st):
    return Data(st, len(st))

def data_to_str(dat):
    return ctypes.string_at(dat.dat, dat.sz)

data_ptr = ctypes.POINTER(Data)

aodbm_lib = ctypes.CDLL("./libaodbm.so")

aodbm_lib.aodbm_open.argtypes = [ctypes.c_char_p]
aodbm_lib.aodbm_open.restype = ctypes.c_void_p

aodbm_lib.aodbm_close.argtypes = [ctypes.c_void_p]
aodbm_lib.aodbm_close.restype = None

aodbm_lib.aodbm_current.argtypes = [ctypes.c_void_p]
aodbm_lib.aodbm_close.restype = ctypes.c_uint64

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
            return data_to_str(ptr.contents)
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

class AODBM(object):
    '''Represents a Database'''
    def __init__(self, filename):
        '''Open a new database'''
        self.db = aodbm_lib.aodbm_open(filename)
    
    def __del__(self):
        aodbm_lib.aodbm_close(self.db)
    
    def current_version(self):
        '''Get the current version object'''
        return Version(self, aodbm_lib.aodbm_current(self.db))
    
    def commit(self, version):
        '''Commits the version object to the database.'''
        assert self == version.db
        return aodbm_lib.aodbm_commit(self.db, version.version)
