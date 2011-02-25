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

tests = [TestSimple]
tests = map(unittest.TestLoader().loadTestsFromTestCase, tests)
tests = unittest.TestSuite(tests)
