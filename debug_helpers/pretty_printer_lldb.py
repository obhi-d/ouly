import lldb
import re

def __lldb_init_module(debugger, internal_dict):
  """Initialize the module when loaded by LLDB."""
  debugger.HandleCommand('type summary add -x "^ouly::small_vector<.*>$" -F pretty_printer_lldb.SmallVectorSummaryProvider')
  debugger.HandleCommand('type synthetic add -x "^ouly::small_vector<.*>$" -l pretty_printer_lldb.SmallVectorSyntheticProvider')
  print('Small vector pretty printer loaded')

def SmallVectorSummaryProvider(valobj, internal_dict):
  """Summary provider for ouly::small_vector."""
  size = valobj.GetNonSyntheticValue().GetChildMemberWithName('size_').GetValueAsUnsigned()
  return f"size={size}"

class SmallVectorSyntheticProvider:
  """Synthetic children provider for ouly::small_vector."""
  
  def __init__(self, valobj, internal_dict):
    self.valobj = valobj
    self.update()
  
  def update(self):
    """Update the internal state."""
    self.size = self.valobj.GetChildMemberWithName('size_').GetValueAsUnsigned()
    
    # Parse the template parameters
    type_name = self.valobj.GetType().GetName()
    match = re.search(r'<([^,]+),\s*(\d+)', type_name)
    
    if match:
      self.element_type_name = match.group(1).strip()
      self.template_inline_cap = int(match.group(2))
      
      # Get the element type
      self.element_type = self.valobj.GetTarget().FindFirstType(self.element_type_name)
      if not self.element_type.IsValid():
        self.element_type = None
        return
        
      element_size = self.element_type.GetByteSize()
      
      # Calculate actual inline capacity
      heap_storage_size = 16  # approximate size of heap_storage
      if self.template_inline_cap == 0:
        self.inline_capacity = max(1, (heap_storage_size + element_size - 1) // element_size)
      else:
        self.inline_capacity = max(self.template_inline_cap, 
                      (heap_storage_size + element_size - 1) // element_size)
      
      # Check if using inline storage
      self.is_inlined = self.size <= self.inline_capacity
  
  def num_children(self):
    """Return the number of children."""
    return self.size
  
  def get_child_at_index(self, index):
    """Return the child at the specified index."""
    if index < 0 or index >= self.size or not self.element_type:
      return None
    
    if self.is_inlined:
      # For inline storage, get the storage array
      ldata = self.valobj.GetChildMemberWithName('data_store_').GetChildMemberWithName('ldata_')
      # Get storage element at index
      base_addr = ldata.GetLoadAddress()

    else:
      # For heap storage
      hdata = self.valobj.GetChildMemberWithName('data_store_').GetChildMemberWithName('hdata_')
      pdata = hdata.GetChildMemberWithName('pdata_')
      base_addr = pdata.GetValueAsUnsigned()
      
    if base_addr == 0:
      return None
    
    # Calculate element address
    element_addr = base_addr + (index * self.element_type.GetByteSize())
    
    # Create a value from this address
    return self.valobj.CreateValueFromAddress(
      f"[{index}]",
      element_addr,
      self.element_type
    )

  def get_child_index(self, name):
    """Get child index from name."""
    try:
      return int(name.lstrip('[').rstrip(']'))
    except:
      return -1
  
  def has_children(self):
    """Return True if this vector has children."""
    return self.size > 0