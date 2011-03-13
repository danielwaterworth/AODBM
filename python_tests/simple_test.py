import unittest, aodbm

class TestSimple(unittest.TestCase):
    def setUp(self):
        self.db = aodbm.AODBM('testdb')
    
    def test_simple(self):
        # a simple test with one record
        ver = aodbm.Version(self.db, 0)
        self.assertRaises(KeyError, ver.__getitem__, 'hello')
        self.assertFalse(ver.has('test'))
        ver['test'] = 'hello'
        self.assertEqual(ver['test'], 'hello')
        self.assertTrue(ver.has('test'))
        del ver['test']
        self.assertRaises(KeyError, ver.__getitem__, 'test')
        self.assertFalse(ver.has('test'))
        del ver['test']
        self.assertRaises(KeyError, ver.__getitem__, 'test')
        self.assertFalse(ver.has('test'))
    
    def test_insert_empty(self):
        ver = aodbm.Version(self.db, 0)
        ver['hello'] = 'world'
        # create an empty root node
        del ver['hello']
        ver['hello'] = 'world'
        self.assertTrue(ver.has('hello'))
        self.assertEqual(ver['hello'], 'world')

tests = [TestSimple]
tests = map(unittest.TestLoader().loadTestsFromTestCase, tests)
tests = unittest.TestSuite(tests)
