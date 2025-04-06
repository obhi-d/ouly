import gdb
import itertools

class SmallVectorPrinter:
    """Pretty printer for ouly::small_vector<T, N>"""

    def __init__(self, val):
        self.val = val
        self.size = int(val['size_'])
        
        # Get the type information
        self.type = val.type
        self.element_type = self.type.template_argument(0)
        self.inline_capacity = int(val.type.template_argument(1))
        
        # Determine if data is stored inline or on heap
        self.is_inlined = (self.size <= self.inline_capacity)


    # Figures out the number of elements in the vector.
    def get_size(self):
        return self.size
    
    def children(self):
        if self.size == 0:
            return []
        
        # Get the data pointer based on storage location
        if self.is_inlined:
            data_ptr = self.val['data_store_']['ldata_'].address
            data_ptr = data_ptr.cast(self.element_type.pointer())
        else:
            data_ptr = self.val['data_store_']['hdata_']['pdata_']
            data_ptr = data_ptr.cast(self.element_type.pointer())
            
        # Generate (index, element) pairs
        for i in range(self.size):
            yield str(i), (data_ptr + i).dereference()

    def to_string(self):
        return "%s of length %d, capacity %d%s" % (
            self.type,
            self.size,
            self.inline_capacity if self.is_inlined else int(self.val['data_store_']['hdata_']['capacity_']),
            " (inline storage)" if self.is_inlined else " (heap storage)"
        )

    def display_hint(self):
        return 'array'

def lookup_pretty_printer(value):
    """Register small_vector pretty-printers with objfile"""
    if value.type.name  and value.type.name.startswith('ouly::small_vector'):
        return SmallVectorPrinter(value)
    return None
# Auto-load when script is imported directly into GDB
gdb.printing.register_pretty_printer(None, lookup_pretty_printer)