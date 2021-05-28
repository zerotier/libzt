import libzt


class select:
    def select(self, r, w, x, timeout=None):
        r, w, x = libzt.zts_py_select(self, r, w, x, timeout)
        return r, w + x, []
