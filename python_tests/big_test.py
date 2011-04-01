'''  
    Copyright (C) 2011 aodbm authors,
    
    This file is part of aodbm.
    
    aodbm is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aodbm is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

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
