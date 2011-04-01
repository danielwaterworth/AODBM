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
