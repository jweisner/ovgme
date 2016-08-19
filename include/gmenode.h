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

#ifndef GMENODE_H_INCLUDED
#define GMENODE_H_INCLUDED

#include "gme.h"
/*
  Oversimplified node class for mod folder/file tree
*/
class GMEnode
{
public:

  /* base constructor */
  GMEnode();

  /* good constructor */
  GMEnode(const std::wstring& name, bool isdir);

  /* destructor */
  ~GMEnode();

  /* get node name (ie. file/folder name) */
  const std::wstring& getName() const {
    return _name;
  }

  /* get node id */
  unsigned getId() const {
    return _id;
  }

  /* get node source (ie. source full path) */
  const std::wstring& getSource() const {
    return _source;
  }

  /* get node path (nrd = exclude root directory) */
  std::wstring getPath(bool nrd=false) const;

  /* get node child */
  GMEnode* getChid(unsigned i) const {
    return _child[i];
  }

  /* get node child count */
  unsigned getChildCount() const {
    return _child.size();
  }

  /* get node parent */
  GMEnode* getParent() const {
    return _parent;
  }

  /* is the node a file or a directory */
  bool isDir() const {
    return _isdir;
  }

  /* check if node has child named... */
  bool hasChild(const std::wstring& name) const;

  /* get node child named... */
  GMEnode* getChild(const std::wstring& name) const;

  /* get node data */
  void* getData() const {
    return _data;
  }

  /* get node data size */
  size_t getDataSize() const {
    return _data_size;
  }

  /* set node name */
  void setName(const std::wstring& name) {
    _name = name;
  }

  /* set node id */
  void setId(unsigned i) {
    _id = i;
  }

  /* set node source (ie. source full path) */
  void setSource(const std::wstring& src)  {
    _source = src;
  }

  /* set parent for this node */
  void setParent(GMEnode* parent);

  /* define as directory or file */
  void setDir(bool dir) {
    _isdir = dir;
  }

  /* set node data */
  bool setData(const void* data, size_t size);

  /* alloc node data */
  bool allocData(size_t size);

  /* init a child depth-first traversal */
  void initTraversal();

  /* sep traversal to next child in tree */
  bool nextChild();

  /* return current child of the traversal */
  GMEnode* currChild() const {
    return _ctcurr;
  }

  /* return current depth of the traversal */
  unsigned currChildDepth() const {
    return _ctdeph;
  }

private:

  std::wstring _name;

  unsigned _id;

  std::wstring _source;

  GMEnode* _parent;

  std::vector<GMEnode*> _child;

  bool _isdir;

  ubyte* _data;

  size_t _data_size;

  GMEnode* _ctcurr;

  unsigned _ctqueue[255];

  unsigned _ctdeph;

  bool _ctended;

};

#endif // GMENODE_H_INCLUDED
