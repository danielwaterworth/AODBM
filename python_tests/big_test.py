import unittest, aodbm, random

class TestBig(unittest.TestCase):
    def setUp(self):
        self.db = aodbm.AODBM('testdb')
    
    def test_big(self):
        ver = aodbm.Version(self.db, 0)
        nums = range(500)
        random.shuffle(nums)
        for n in nums:
            ver['hello' + str(n)] = 'world' + str(n)
        random.shuffle(nums)
        for i in nums:
            self.assertEqual(ver['hello' + str(i)], 'world' + str(i))
        random.shuffle(nums)
        for n in nums:
            del ver['hello' + str(n)]
            self.assertRaises(KeyError, ver.__getitem__, 'hello' + str(n))

tests = [TestBig]
tests = map(unittest.TestLoader().loadTestsFromTestCase, tests)
tests = unittest.TestSuite(tests)
