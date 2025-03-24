import gdb
import gdb.printing

class AclSmallVectorPrinter:
  """Pretty printer for acl::small_vector"""

  def __init__(self, val):
    self.val = val
    self.size = int(self.val['size_'])
    self.inline_capacity = int(self.val.type.template_argument(1))
    self.is_inlined = self.size <= self.inline_capacity
    self.element_type = self.val.type.template_argument(0)
  
  def to_string(self):
    return f"{self.val.type} of length {self.size}, capacity {self.get_capacity()}, {'inlined' if self.is_inlined else 'heap allocated'}"
  
  def get_capacity(self):
    if self.is_inlined:
      return self.inline_capacity
    else:
      return int(self.val['data_store_']['hdata_']['capacity_'])
  
  def children(self):
    if self.size == 0:
      return []
    
    if self.is_inlined:
      # For inline storage, access the ldata_ array
      storage_array = self.val['data_store_']['ldata_']
      # Create a view of the array as the element type
      data_ptr = storage_array[0].address.cast(self.element_type.pointer())
    else:
      # For heap storage, access the pdata_ pointer
      storage_ptr = self.val['data_store_']['hdata_']['pdata_']
      # Create a view of the memory as the element type
      data_ptr = storage_ptr.cast(self.element_type.pointer())
    
    return [('[%d]' % i, data_ptr[i]) for i in range(self.size)]

  def display_hint(self):
    return 'array'

def register_acl_printers(obj=None):
  """Register ACL pretty-printers with GDB"""
  
  if obj is None:
    obj = gdb.current_objfile()
  
  printer = gdb.printing.RegexpCollectionPrettyPrinter("acl-containers")
  printer.add_printer('acl::small_vector', '^acl::small_vector<.*>$', AclSmallVectorPrinter)
  
  gdb.printing.register_pretty_printer(obj, printer)

# Register the pretty printer
if __name__ == '__main__':
  register_acl_printers()