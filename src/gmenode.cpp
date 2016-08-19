/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/
 */

#include "gmenode.h"

GMEnode::GMEnode()
{
  _id = 0;
  _parent = NULL;
  _isdir = true;
  _data = NULL;
  _data_size = 0;
}


GMEnode::GMEnode(const std::wstring& name, bool isdir)
{
  _name = name;
  _id = 0;
  _parent = NULL;
  _isdir = isdir;
  _data = NULL;
  _data_size = 0;
}


GMEnode::~GMEnode()
{
  if(_data) {
    delete [] _data;
    _data = NULL;
    _data_size = 0;
  }
  for(unsigned i= 0 ; i < _child.size(); i++)
    delete _child[i];
}


std::wstring GMEnode::getPath(bool nrd) const
{
  if(_parent) {
    return _parent->getPath(nrd) + L"\\" + _name;
  } else {
    if(nrd) {
      return std::wstring();
    } else {
      return _name;
    }
  }
}

void GMEnode::setParent(GMEnode* parent)
{
  unsigned i;
  //verifie si le nouveau parent est déjà un enfant de this
  if(parent != NULL) {
    i = _child.size();
    while(i--) if(_child[i] == parent) { _child.erase(_child.begin()+i); break; }
  }

  // On enleve this de la liste des enfants de son actuel parent
  if(_parent != NULL) {
    i = _parent->_child.size();
    while(i--) if(_parent->_child[i] == this) { _parent->_child.erase(_parent->_child.begin()+i); break; }
  }

  _parent = parent;
  if(_parent != NULL)
    _parent->_child.push_back(this);

}

bool GMEnode::hasChild(const std::wstring& name) const
{
  for(unsigned i = 0; i < _child.size(); i++) {
    if(_child[i]->_name == name)
      return true;
  }
  return false;
}

GMEnode* GMEnode::getChild(const std::wstring& name) const
{
  for(unsigned i = 0; i < _child.size(); i++) {
    if(_child[i]->_name == name)
      return _child[i];
  }
  return NULL;
}

bool GMEnode::allocData(size_t size)
{
  if(_data) {
    delete [] _data;
    _data = NULL;
  }

  try {
    _data = new ubyte[size];
  } catch (const std::bad_alloc&) {
    _data = NULL;
    return false;
  }
  if(_data) {
    _data_size = size;
  }
  return true;
}

bool GMEnode::setData(const void* data, size_t size)
{
  if(_data) {
    delete [] _data;
    _data = NULL;
  }

  try {
    _data = new ubyte[size];
  } catch (const std::bad_alloc&) {
    _data = NULL;
    return false;
  }
  if(_data) {
    _data_size = size;
    memcpy(_data, data, _data_size);
  }
  return true;
}

void GMEnode::initTraversal()
{
  _ctcurr = this;
  memset(&_ctqueue, 0, sizeof(unsigned)*255);
  _ctdeph = 0;
  _ctended = false;
}

bool GMEnode::nextChild()
{
  if (!_ctended) {
    // Remonte l'arbre si tous les enfants du node on été visités
    while (_ctqueue[_ctdeph] == _ctcurr->_child.size()) {
      _ctqueue[_ctdeph] = 0;
      if (_ctdeph == 0) {
        _ctended = true;
        return false;
      }
      _ctdeph--;
      _ctcurr = _ctcurr->_parent;
    }
    // Passe à l'enfant suivant
    _ctcurr = _ctcurr->_child[_ctqueue[_ctdeph]];
    _ctqueue[_ctdeph]++; _ctdeph++;
  }
  return !_ctended;
}
