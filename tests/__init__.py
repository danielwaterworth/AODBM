import unittest
import simple_test
import big_test

tests = unittest.TestSuite([simple_test.tests, big_test.tests])
