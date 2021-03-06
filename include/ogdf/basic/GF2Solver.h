/** \file
* \brief Defines class GF2Solver, which represents a solver for
*        linear equation systems over GF(2).
*
* \author Carsten Gutwenger
*
* \par License:
* This file is part of the Open Graph Drawing Framework (OGDF).
*
* \par
* Copyright (C)<br>
* See README.txt in the root directory of the OGDF installation for details.
*
* \par
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* Version 2 or 3 as published by the Free Software Foundation;
* see the file LICENSE.txt included in the packaging of this file
* for details.
*
* \par
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* \par
* You should have received a copy of the GNU General Public
* License along with this program; if not, write to the Free
* Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*
* \see  http://www.gnu.org/copyleft/gpl.html
***************************************************************/

#pragma once

#include <ogdf/basic/List.h>
#include <iomanip>


namespace ogdf {

class GF2Solver {

	enum { chunkSize = 13, chunkSize2 = 9 };

	struct Chunk {
		int m_x[chunkSize];
		int m_max;
		Chunk *m_next;

		Chunk() {
			m_max = -1;
			m_next = nullptr;
		}

		bool full() const {
			return m_max == chunkSize-1;
		}

		void add(int x) {
			m_x[++m_max] = x;
		}

		OGDF_NEW_DELETE
	};

	struct Chunk2 {
		int               m_x [chunkSize2];
		ListIterator<int> m_it[chunkSize2];
		int m_max;
		Chunk2 *m_next;

		Chunk2() {
			m_max = -1;
			m_next = nullptr;
		}

		bool full() const {
			return m_max == chunkSize2-1;
		}

		void add(int x, ListIterator<int> it) {
			++m_max;
			m_x [m_max] = x;
			m_it[m_max] = it;
		}

		OGDF_NEW_DELETE
	};

	//struct Row {
	//	Chunk *m_pHead;
	//	Chunk *m_pTail;

	//	Row() {
	//		m_pHead = m_pTail = nullptr;
	//	}

	//	void addChunk(Chunk *p) {
	//		if(m_pHead == nullptr)
	//			m_pHead = m_pTail = p;
	//		else {
	//			m_pTail->m_next = p;
	//			m_pTail = p;
	//		}
	//	}
	//};

	struct Row2 {
		Chunk2 *m_pHead;
		Chunk2 *m_pTail;

		Row2() {
			m_pHead = m_pTail = nullptr;
		}

		void addChunk(Chunk2 *p) {
			if(m_pHead == nullptr)
				m_pHead = m_pTail = p;
			else {
				m_pTail->m_next = p;
				m_pTail = p;
			}
		}
	};

	Chunk *m_freelist;
	Chunk2 *m_freelist2;

	//Chunk *getChunk() {
	//	if(m_freelist != nullptr) {
	//		Chunk *p = m_freelist;
	//		m_freelist = p->m_next;
	//		p->m_next = nullptr;
	//		p->m_max = -1;
	//		return p;
	//	}
	//	return new Chunk;
	//}

	Chunk2 *getChunk2() {
		if(m_freelist2 != nullptr) {
			Chunk2 *p = m_freelist2;
			m_freelist2 = p->m_next;
			p->m_next = nullptr;
			p->m_max = -1;
			return p;
		}
		return new Chunk2;
	}

	//void freeChunk(Chunk *p) {
	//	p->m_next = m_freelist;
	//	m_freelist = p;
	//}

	void freeChunk2(Chunk2 *p) {
		p->m_next = m_freelist2;
		m_freelist2 = p;
	}

	//void freeChunks(Chunk *pHead, Chunk *pTail) {
	//	pTail->m_next = m_freelist;
	//	m_freelist = pHead;
	//}

	void freeChunks2(Chunk2 *pHead, Chunk2 *pTail) {
		pTail->m_next = m_freelist2;
		m_freelist2 = pHead;
	}

	//bool contains(const Row &r, int x) const;

	//void symDiff(Row &r, const Row &other);
	void symDiff2(int r1, int r2, Array<Row2> &rows, Array<List<int>> &cols);

public:

	class Equation {

		List<int> m_objects;

	public:
		Equation() { }

		void print() {
			cout << m_objects << endl;
		}

		ListConstIterator<int> begin() const { return m_objects.begin(); }
		ListConstIterator<int> end() const { return m_objects.end(); }

		//bool contains(OBJ obj) const {
		//	for(OBJ x : m_objects) {
		//		if(x == obj)
		//			return true;
		//		else if(x > obj)
		//			return false;
		//	}
		//	return false;
		//}

		int size() const {
			return m_objects.size();
		}

		Equation &operator|=(int obj) {
			ListIterator<int> it = m_objects.begin();
			while(it.valid() && *it < obj)
				++it;
			if(it.valid()) {
				if(*it != obj)
					m_objects.insertBefore(obj,it);
			} else
				m_objects.pushBack(obj);

			return *this;
		}

		//Equation &operator^=(const Equation &other) {
		//	ListConstIterator<OBJ> itOther = other.m_objects.begin();
		//	ListIterator<OBJ> it = m_objects.begin();

		//	while(itOther.valid())
		//	{
		//		if(!it.valid()) {
		//			m_objects.pushBack(*itOther);
		//			++itOther;

		//		} else if(*it == *itOther) {
		//			ListIterator<OBJ> itDel = it;
		//			++it; ++itOther;
		//			m_objects.del(itDel);

		//		} else if(*itOther < *it) {
		//			m_objects.insertBefore(*itOther, it);
		//			++itOther;

		//		} else {
		//			++it;
		//		}
		//	}

		//	return *this;
		//}

		OGDF_NEW_DELETE
	};

	class Matrix {

		Array<Equation*> m_equations;
		int m_numRows;
		int m_numCols;

	public:
		Matrix() : m_equations(0, 255, nullptr), m_numRows(0), m_numCols(0) { }

		~Matrix() {
			for(int i = 0; i < m_numRows; ++i)
				delete m_equations[i];
		}

		Equation &operator[](int i) {
			OGDF_ASSERT(i >= 0);
			OGDF_ASSERT(i < m_numRows);
			return *m_equations[i];
		}

		const Equation &operator[](int i) const {
			OGDF_ASSERT(i >= 0);
			OGDF_ASSERT(i < m_numRows);
			return *m_equations[i];
		}

		int numRows() const { return m_numRows; }
		int numColumns() const { return m_numCols; }

		int addRow() {
			int i = m_numRows++;
			if(i == m_equations.size())
				m_equations.grow(m_equations.size(), nullptr);
			m_equations[i] = new Equation;

			return i;
		}

		int addColumn() {
			return m_numCols++;
		}

		void clear() {
			for(int i = 0; i < m_numRows; ++i)
				delete m_equations[i];

			m_equations.init(0, 255, nullptr);
			m_numRows = m_numCols = 0;
		}

		void print() const {
			for(int i = 0; i < m_numRows; ++i) {
				cout << std::setw(4) << i << ": ";
				m_equations[i]->print();
			}
		}
	};


	GF2Solver(GF2Solver::Matrix &Mx)
	  : m_freelist(nullptr)
	  , m_freelist2(nullptr)
	  , m_matrix(Mx)
	{
	}

	~GF2Solver();

	bool solve();
	bool solve2();


private:
	Matrix &m_matrix;
};

}
