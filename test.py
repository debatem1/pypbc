#! /usr/bin/env python3

"""
test.py

Written by Geremy Condra
Licensed under GPLv3
Released 11 October 2009
"""

import unittest

from pypbc import *

stored_params = """type a
q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791
h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776
r 730750818665451621361119245571504901405976559617
exp2 159
exp1 107
sign1 1
sign0 1
"""

class TestParameters(unittest.TestCase):
	
	def setUp(self): pass
	
	def test_init(self):
		try:
			# test n-type generation
			params = Parameters(n=3559*3571) 	# type a1
			# not tested because it is *crazy* slow on my old machine...
			#params = Parameters(n=3559*3571, short=True) # type f
			# test s-type generation
			params = Parameters(param_string=stored_params)	# type a
			# test qr-type generation
			params = Parameters(qbits=512, rbits=160) # type a
			#params = Parameters(qbits=512, rbits=160, short=True) # type e, may be renamed
		except Exception:
			fail("Could not instantiate parameters")
			
	def test_bad_init(self):
		# here we try to make it screw up by calling it incorrectly
		# here, param_string and other values are given
		try:
			Parameters(param_string=stored_params, short=True)
			self.fail()
		except: pass
		try:
			Parameters(param_string=stored_params, qbits=512)
			self.fail()
		except: pass
		try:
			Parameters(param_string=stored_params, rbits=512)
			self.fail()
		except: pass
		try:
			Parameters(param_string=stored_params, n=512)
			self.fail()
		except: pass
		try:
			Parameters(param_string=stored_params, rbits=512)
			self.fail()
		except: pass
		#self.assertRaises(Exception, Parameters, "hello world")
		self.assertRaises(Exception, Parameters, 1.0)
		pass

class TestPairing(unittest.TestCase):

	def setUp(self):
		self.params = Parameters(n=3559*3571)
		
	def test_init(self):
		try:
			Pairing(self.params)
		except Exception:
			self.fail("Could not instantiate pairing")

	def test_bad_init(self):
		#self.assertRaises(Exception, Pairing)
		self.assertRaises(Exception, Pairing, "hello world")
		
	def test_apply(self):
		pairing = Pairing(self.params)
		e1 = Element(pairing, G1)
		e2 = Element(pairing, G2)
		try:
			e3 = pairing.apply(e1, e2)
		except:
			self.fail("could not apply pairing.")
			
	def test_bad_apply(self):
		pairing = Pairing(self.params)
		e1 = Element(pairing, G1)
		e2 = Element(pairing, G2)
		self.assertRaises(Exception, pairing.apply)
		self.assertRaises(Exception, pairing.apply, e1)
		self.assertRaises(Exception, pairing.apply, "hi", 1.5)

			
class TestElement(unittest.TestCase):

	def setUp(self):
		self.params = Parameters(n=3559*3571)
		self.pairing = Pairing(self.params)
		
	def test_init(self):
		try:
			self.e1 = Element(self.pairing, G1)
			self.e2 = Element.zero(self.pairing, G2)
			self.e3 = Element.one(self.pairing, G1)
			self.e4 = Element.random(self.pairing, G1)
			self.e5 = Element(self.pairing, Zr, value=3559)
			self.e6 = Element.from_hash(self.pairing, Zr, "hello world!")
			print(self.e6)
		except Exception:
			self.fail("Could not instantiate element")
	 
	def test_str(self):
		self.e5 = Element(self.pairing, Zr, value=3559)
		self.assertEqual("3559", str(self.e5))

	def test_bad_init(self):
		self.assertRaises(TypeError, Element)
		self.assertRaises(TypeError, Element, self.pairing)
		self.assertRaises(TypeError, Element, G1)
		self.assertRaises(TypeError, Element, "hidey ho", G1)
		self.assertRaises(TypeError, Element, self.pairing, "snerk")
		try:
			Element.random(self.pairing, GT)
			self.fail()
		except: pass

	def test_add(self):
		self.e1 = Element(self.pairing, Zr, value=3)
		self.e2 = Element(self.pairing, Zr, value=5)
		self.e3 = self.e1 + self.e2
		self.assertEqual(str(self.e3), "8")
		self.e4 = self.e1 + self.e3
		
	def test_sub(self):
		self.e1 = Element(self.pairing, Zr, value=3)
		self.e2 = Element(self.pairing, Zr, value=5)
		self.e3 = self.e2 - self.e1
		self.assertEqual(str(self.e3), "2")
		
	def test_mult(self):
		self.e1 = Element(self.pairing, Zr, value=3)
		self.e2 = Element(self.pairing, Zr, value=5)
		self.e3 = self.e1 * self.e2
		self.assertEqual(str(self.e3), "15")
		try: self.e3 * 5
		except: self.fail()
		self.e4 = self.pairing.apply(Element.random(self.pairing, G1), Element.random(self.pairing, G2))
		self.e5 = self.pairing.apply(Element.random(self.pairing, G1), Element.random(self.pairing, G2))
		try: self.e4 * self.e5
		except: self.fail()
		
	def test_pow(self):
		self.e1 = Element(self.pairing, Zr, value=3)
		self.e2 = Element(self.pairing, Zr, value=2)
		self.e3 = pow(self.e1, self.e2)
		self.assertEqual(str(self.e3), "9")
		try: 
			self.e1**(1,2)
			self.fail()
		except: 
			pass
		
	def test_cmp(self):
		self.e1 = Element.random(self.pairing, G1)
		self.e2 = Element.random(self.pairing, G1)
		self.assertEqual(self.e1 == self.e2, False)
		self.e3 = Element(self.pairing, Zr, value=36)
		self.e4 = Element(self.pairing, Zr, value=36)
		self.assertEqual(self.e3, self.e4)
		self.e5 = Element.zero(self.pairing, G1);
		self.e6 = Element.one(self.pairing, Zr);
		self.assertEqual(self.e5 == 0, True)
		self.assertEqual(self.e5 == 1, True)
		self.assertEqual(self.e6 == 0, False)
		self.assertEqual(self.e6 == 1, True)
		self.e7 = Element.random(self.pairing, G2)
		self.assertEqual(self.e7 == 0, False)
		self.assertEqual(self.e7 == 1, False)		
		try:
			self.e5 < self.e6
			self.fail()
			self.e5 > self.e6
			self.fail()
		except: pass
		
	def test_neg(self):
		self.e1 = Element.random(self.pairing, Zr)
		self.e2 = -self.e1
		try:
			-Element.one(self.pairing, GT)
			self.fail()
		except: pass
		
	def test_inv(self):
		self.e1 = Element.random(self.pairing, Zr)
		self.e2 = ~self.e1
		try:
			~Element.one(self.pairing, GT)
			self.fail()
		except: pass
		
	def test_bls(self):
		stored_params = """type a
		q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791
		h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776
		r 730750818665451621361119245571504901405976559617
		exp2 159
		exp1 107
		sign1 1
		sign0 1
		"""

		# this is a test for the BLS short signature system
		params = Parameters(param_string=stored_params)
		pairing = Pairing(params)

		# build the common parameter g
		g = Element.random(pairing, G2)

		# build the public and private keys
		private_key = Element.random(pairing, Zr)
		public_key = Element(pairing, G2, value=g**private_key)

		# set the magic hash value
		hash_value = Element.from_hash(pairing, G1, "hashofmessage")

		# create the signature
		signature = hash_value**private_key

		# build the temps
		temp1 = Element(pairing, GT)
		temp2 = Element(pairing, GT) 

		# fill temp1
		temp1 = pairing.apply(signature, g)

		#fill temp2
		temp2 = pairing.apply(hash_value, public_key)

		# and again...
		temp1 = pairing.apply(signature, g)

		# compare
		self.assertEqual(temp1 == temp2, True)

		# compare to random signature
		rnd = Element.random(pairing, G1)
		temp1 = pairing.apply(rnd, g)

		# compare
		self.assertEqual(temp1 == temp2, False)
		
	def test_auth(self):
		# taken from paul miller's PBC::Crypt module tutorial
		params = Parameters(qbits=512, rbits=160)
		pairing = Pairing(params)
		p = Element.random(pairing, G2)
		s = Element.random(pairing, Zr)
		public_key = p**s
		# not actually from a hash. I don't really care.
		Q_0 = Element.from_hash(pairing, G1, "node_id=22609")
		# ditto
		Q_1 = Element.from_hash(pairing, G1, "node_id=9073")
		d_0 = Q_0**s
		d_1 = Q_1**s
		k_01_a = pairing.apply(Q_0, d_1)
		k_01_b = pairing.apply(d_0, Q_1)
		self.assertEqual(k_01_a, k_01_b)
		k_10_a = pairing.apply(Q_1, d_0)
		k_10_b = pairing.apply(d_1, Q_0)
		self.assertEqual(k_10_a, k_10_b)
		

if __name__ == '__main__':
	# unittest.main()
	params = Parameters(qbits=128, rbits=100)
	pairing = Pairing(params)
	P = Element.random(pairing, G1)
	Q = Element.random(pairing, G2)
	r = Element.random(pairing, Zr)
	e = pairing.apply(P, Q)
	print("params =", str(params))
	print("pairing =", str(pairing))
	print("P =", str(P))
	PP = Element(pairing, G1, value=str(P))
	print("PP =", str(PP))
	assert P == PP
	print("Q =", str(Q))
	print("r =", str(r))
	print("e =", str(e))
	ee = Element(pairing, GT, value=str(e))
	print("ee =", str(ee))
	print("len(P) =", len(P))
	print("P[0] = 0x%X" % (P[0]))
	print("P[1] = 0x%X" % (P[1]))
	assert len(P) == 2
	print("len(Q) =", len(Q))
	print("Q[0] = 0x%X" % (Q[0]))
	print("Q[1] = 0x%X" % (Q[1]))
	assert len(P) == 2
	try:
		print("len(r) =", len(r))
		assert False
	except TypeError:
		print(" failed as expected")
	print("int(r) = ", int(r))
	print("int(r), hex = 0x%X" % (int(r)))
	print("len(e) =", len(e))
	print("e[0] = 0x%X" % (e[0]))
	print("e[1] = 0x%X" % (e[1]))
	assert len(P) == 2
	assert e == ee
	P0 = Element.zero(pairing, G1)
	P1 = Element.one(pairing, G1)
	print("P0 =", str(P0))
	print("P1 =", str(P1))
	Q0 = Element.zero(pairing, G2)
	Q1 = Element.one(pairing, G2)
	print("Q0 =", str(Q0))
	print("Q1 =", str(Q1))
	rP = P * r
	print("rP =", str(rP))
	for j in range (0,2):
		
		if j == 0:
			set_point_format_compressed()
		else:
			set_point_format_uncompressed()
		for i in range(0, 100):
			R = Element.random(pairing, G1)
			RR = Element(pairing, G1, str(R))
			S = Element.random(pairing, G1)
			e = pairing.apply(P, Q)
			ee = Element(pairing, GT, value=str(e))
			if i < 10:
				print ("R =", str(R))
				print ("RR =", str(R))
				print ("S =", str(S))
				print("e =", str(e))
				print("ee =", str(ee))
			assert RR == R
			assert ee == e
	# for i in range(0,1000):
	# 	r = Element.random(pairing, Zr)
	# 	print("i, r =", i, str(r))
