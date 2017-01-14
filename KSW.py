#! /usr/bin/env python3

"""
KSW.py

Written by Geremy Condra
Licensed under GPLv3
Released 15 October 2009

An implementation of the Katz-Sahai-Waters predicate
encryption scheme in Python3.
"""

from pypbc import *

#############################################
#					Utilities								     #
#############################################

def dot_product(x, y, n):
	"""Takes the dot product of lists x and y over F_n"""
	
	if len(x) != len(y):
		raise ValueError("x and y must be the same length!")
	if not isinstance(n, int):
		raise ValueError("n must be an integer!")
	
	return sum([(x_i * y[i]) % n for i, x_i in enumerate(x)])

#############################################
#						Cryptosystem						     #
#############################################	

class PublicKey:
	g_G_p = None
	g_G_r = None
	Q = None
	vector = None
	def __init__(self, gen_p, gen_r, Q, vector):
		self.g_G_p = gen_p
		self.g_G_r = gen_r
		self.Q = Q
		self.vector = vector
	
class MasterSecretKey:
	p = None
	q = None
	r = None
	g_G_q = None
	Hs = None
	def __init__(self, p, q, r, g_G_q, Hs):
		self.p = p
		self.q = q
		self.r = r
		self.g_G_q = g_G_q
		self.Hs = Hs


class Cryptosystem:
	
	def __init__(self, security) -> "(PK, SK)":
		self.security = security
		# select p, q, r
		p = get_random_prime(100)
		q = get_random_prime(100)
		r = get_random_prime(100)
		# make n
		self.n = p*q*r
		# build the params
		params = Parameters(n=self.n)
		# build the pairing
		self.pairing = Pairing(params)
		# find the generators for the G_p, G_q, and G_r subgroups
		g_G_p = Element.random(self.pairing, G1)**(q*r)
		g_G_r = Element.random(self.pairing, G1)**(p*q)
		g_G_q = Element.random(self.pairing, G1)**(p*r)
		# choose R0
		R0 = g_G_r ** Element.random(self.pairing, Zr)
		# choose the random R's
		Rs = [(g_G_r**Element.random(self.pairing, Zr), g_G_r**Element.random(self.pairing, Zr)) for i in range(security)]
		hs = [(g_G_p**Element.random(self.pairing, Zr), g_G_p**Element.random(self.pairing, Zr)) for i in range(security)]
		# choose the random H's
		Hs = []
		for i in range(security):
			Hs.append((hs[i][0] * Rs[i][0], hs[i][1] * Rs[i][1]))
		# calculate Q
		Q = g_G_q * R0
		# build the public and master secret keys
		self.pk = PublicKey(g_G_p, g_G_r, Q, Hs)
		self.sk = MasterSecretKey(p, q, r, g_G_q, hs)


	def keygen(self, v: "description of a predicate") -> "SK_f":
		R5 = self.pk.g_G_r**Element.random(self.pairing, Zr)
		Q6 = self.sk.g_G_q**Element.random(self.pairing, Zr)
		Rs = []
		for i in range(self.security):
			# build r1
			r1 = Element(self.pairing, Zr, get_random(self.sk.p))
			# build r2
			r2 = Element(self.pairing, Zr, get_random(self.sk.p))
			Rs.append((r1, r2))
		f1 = Element(self.pairing, Zr, get_random(self.sk.q))
		f2 = Element(self.pairing, Zr, get_random(self.sk.q))
		K = R5*Q6
		for pos in range(self.security):
			# get h1, h2
			h1, h2 = self.sk.Hs[pos]
			# get r1, r2
			r1, r2 = Rs[pos]
			# form the intermediate value
			i1 = h1**(-r1)
			i2 = h2**(-r2)
			K += i1 * i2
		Ks = []
		for pos in range(self.security):
			r1, r2 = Rs[pos]
			K1 = (self.pk.g_G_p**r1) * (self.sk.g_G_q**(f1*v[pos]))
			K2 = (self.pk.g_G_p**r2) * (self.sk.g_G_q**(f2*v[pos]))
			Ks.append((K1, K2))
		return (K, Ks)
		
	
	def encrypt(self, x: "vector of elements in Zr") -> "ciphertext":
		s = Element.random(self.pairing, Zr) 
		a = Element.random(self.pairing, Zr)
		b = Element.random(self.pairing, Zr)
		Rs = []
		for i in range(self.security):
			r1 = self.pk.g_G_r**Element.random(self.pairing, Zr)
			r2 = self.pk.g_G_r**Element.random(self.pairing, Zr)
			Rs.append((r1, r2))
		C0 = self.pk.g_G_p**s
		Cs = []
		for i in range(self.security):
			c1i = (self.pk.vector[i][0]**s)
			c1i2 = (self.pk.Q**(a*x[i]))
			c1 = c1i*c1i2*Rs[i][0]
			c2i = (self.pk.vector[i][1]**s)
			c2i2 = (self.pk.Q**(b*x[i]))
			c2 = c2i*c2i2*Rs[i][1]
			Cs.append((c1, c2))
		return (C0, Cs)
	
	def decrypt(self, c: "ciphertext", sk_f: "secret key corresponding to predicate f") -> "message or T":
		output = self.pairing.apply(c[0], sk_f[0])
		for i in range(self.security):
			j = self.pairing.apply(c[1][i][0], sk_f[1][i][0])
			k = self.pairing.apply(c[1][i][1], sk_f[1][i][1])
			output *= j*k
		return output

#############################################
#						Test logic							     #
#############################################

def test():
	# build the polynomial vector
	Pv = [1, -27, 152] # = X^2 - 27X + 152 = (X - 8)(X - 19)
	# build the X vector
	Xv = [19**3, 19**2, 19**1] 
	# build the random primes
	p = get_random_prime(100)
	q = get_random_prime(100)
	r = get_random_prime(100)
	n = p*q*r
	# build the parameters
	params = Parameters(n=n)
	# build the pairing
	pairing = Pairing(params)
	# get the generators for G_p, G_q, G_r
	g_G_p = Element.random(pairing, G1)**q*r
	g_G_q = Element.random(pairing, G1)**p*r
	g_G_r = Element.random(pairing, G1)**p*q
	# test the generators
	assert(pairing.apply(g_G_p, g_G_r) == 1)
	assert(pairing.apply(g_G_r, g_G_q) == 1)
	# select the random integers from Zn
	a = Element.random(pairing, Zr)
	b = Element.random(pairing, Zr)
	# get random integers from Zq
	f1 = Element(pairing, Zr, get_random(q))
	f2 = Element(pairing, Zr, get_random(q))
	# perform the check
	result = Element.zero(pairing, GT)
	for pos, i in enumerate(Pv):
		result += pairing.apply(g_G_q, g_G_q)**(((a*f1+b*f2)) * Xv[pos]*i)
	assert(result == 1)
	# work backwards one step
	# make s
	s = Element.random(pairing, Zr)
	# make the h vector
	hv = [(g_G_p**Element.random(pairing, Zr), g_G_p**Element.random(pairing, Zr)) for i in range(3)]
	# make the r vector
	rv = [(Element(pairing, Zr, get_random(p)), Element(pairing, Zr, get_random(p))) for i in range(3)]
	# perform the hv<>rv product operation
	product = Element.one(pairing, G1)
	for pos, i in enumerate(hv):
		h1, h2 = i
		r1, r2 = rv[pos]
		product *= (h1**-r1)*(h2**-r2)
	# get the initial result
	result = pairing.apply(g_G_p**s, product)
	# perform the secondary product operation
	for pos, i in enumerate(hv):
		h1, h2 = i
		r1, r2 = rv[pos]
		x = Xv[pos]
		v = Pv[pos]
		arg1 = (h1**s)*(g_G_q**(a*x))
		arg2 = (g_G_p**r1)*(g_G_q**(f1*v))
		part1 = pairing.apply(arg1, arg2)
		arg1 = (h2**s)*(g_G_q**(b*x))
		arg2 = (g_G_p**r2)*(g_G_q**(f2*v))
		part2 = pairing.apply(arg1, arg2)
		result += part1*part2
	assert(result == 1)
	# work backwards another step
	# build R0
	R0 = g_G_r**Element.random(pairing, Zr)
	# build Q
	Q = g_G_q * R0
	# build R5
	R5 = g_G_r**Element.random(pairing, Zr)
	# build Q6
	Q6 = g_G_q**Element.random(pairing, Zr)
	# build the R vector
	Rv = [(g_G_r**Element.random(pairing, Zr), g_G_r**Element.random(pairing, Zr)) for i in range(3)]
	# build the H vector
	Hv = []
	for i in range(3):
		Hv.append((hv[i][0] * Rv[i][0], hv[i][1] * Rv[i][1]))
	# build the initial pairing value
	product = Element.one(pairing, G1)
	for pos, i in enumerate(hv):
		h1, h2 = i
		r1, r2 = rv[pos]
		product *= (h1**-r1)*(h2**-r2)
	# get the initial result
	result = pairing.apply(g_G_p**s, R5*Q6*product)
	for pos, i in enumerate(Hv):
		H1, H2 = i
		R3, R4 = Rv[pos]
		r1, r2 = rv[pos]
		part1 = pairing.apply((H1**s)*(Q**(a*Xv[pos])*R3), (g_G_p**r1)*(g_G_q**(f1*Pv[pos])))
		part2 = pairing.apply((H2**s)*(Q**(b*Xv[pos])*R4), (g_G_p**r2)*(g_G_q**(f2*Pv[pos])))
		result *= part1*part2
	assert(result == 1)
	# done with the proof, test the cryptosystem against it
	c = Cryptosystem(3)
	# build the secret key corresponding to the above polynomial
	skf = c.keygen(Pv)
	# encrypt the value given
	e = c.encrypt(Xv)
	# decrypt it
	assert(c.decrypt(e, skf) == 1)

if __name__ == "__main__":
	test()
