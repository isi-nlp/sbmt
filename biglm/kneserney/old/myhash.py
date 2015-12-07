def myhash(key, modulus):
    result = 0
    for c in key:
        result = (result << 1) ^ ord(c)
    return result%modulus
