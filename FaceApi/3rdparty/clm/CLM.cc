///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2010, Jason Mora Saragih, all rights reserved.
//
// This file is part of FaceTracker.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * The software is provided under the terms of this licence stricly for
//       academic, non-commercial, not-for-profit purposes.
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions (licence) and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions (licence) and the following disclaimer
//       in the documentation and/or other materials provided with the
//       distribution.
//     * The name of the author may not be used to endorse or promote products
//       derived from this software without specific prior written permission.
//     * As this software depends on other libraries, the user must adhere to
//       and keep in place any licencing terms of those libraries.
//     * Any publications arising from the use of this software, including but
//       not limited to academic journal and conference publications, technical
//       reports and manuals, must cite the following work:
//
//       J. M. Saragih, S. Lucey, and J. F. Cohn. Face Alignment through
//       Subspace Constrained Mean-Shifts. International Conference of Computer
//       Vision (ICCV), September, 2009.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "CLM.h"
#include "Framework/Profiler.h"

#include <iostream>
#include <opencv2/imgproc.hpp>

#define it at<int>
#define db at<double>
#define SQR(x) x*x

using namespace FACETRACKER;
using namespace std;

namespace
{
	void get_quadrangle_sub_pix_8u32f(const uchar* iSrc, size_t iSrcStep, const cv::Size& iSrcSize,
		float* oDst, size_t iDstStep, const cv::Size& iWinSize,
		const double *iMatrix, int iCn)
	{
		int x, y, k;
		double A11 = iMatrix[0], A12 = iMatrix[1], A13 = iMatrix[2];
		double A21 = iMatrix[3], A22 = iMatrix[4], A23 = iMatrix[5];

		iSrcStep /= sizeof(iSrc[0]);
		iDstStep /= sizeof(oDst[0]);

		for (y = 0; y < iWinSize.height; y++, oDst += iDstStep)
		{
			double xs = A12 * y + A13;
			double ys = A22 * y + A23;
			double xe = A11 * (iWinSize.width - 1) + A12 * y + A13;
			double ye = A21 * (iWinSize.width - 1) + A22 * y + A23;

			if ((unsigned)(cvFloor(xs) - 1) < (unsigned)(iSrcSize.width - 3) &&
				(unsigned)(cvFloor(ys) - 1) < (unsigned)(iSrcSize.height - 3) &&
				(unsigned)(cvFloor(xe) - 1) < (unsigned)(iSrcSize.width - 3) &&
				(unsigned)(cvFloor(ye) - 1) < (unsigned)(iSrcSize.height - 3))
			{
				for (x = 0; x < iWinSize.width; x++)
				{
					int ixs = cvFloor(xs);
					int iys = cvFloor(ys);
					const uchar *ptr = iSrc + iSrcStep * iys;
					float a = (float)(xs - ixs), b = (float)(ys - iys), a1 = 1.f - a, b1 = 1.f - b;
					float w00 = a1 * b1, w01 = a * b1, w10 = a1 * b, w11 = a * b;
					xs += A11;
					ys += A21;

					if (iCn == 1)
					{
						ptr += ixs;
						oDst[x] = ptr[0] * w00 + ptr[1] * w01 + ptr[iSrcStep] * w10 + ptr[iSrcStep + 1] * w11;
					}
					else if (iCn == 3)
					{
						ptr += ixs * 3;
						float t0 = ptr[0] * w00 + ptr[3] * w01 + ptr[iSrcStep] * w10 + ptr[iSrcStep + 3] * w11;
						float t1 = ptr[1] * w00 + ptr[4] * w01 + ptr[iSrcStep + 1] * w10 + ptr[iSrcStep + 4] * w11;
						float t2 = ptr[2] * w00 + ptr[5] * w01 + ptr[iSrcStep + 2] * w10 + ptr[iSrcStep + 5] * w11;

						oDst[x * 3] = t0;
						oDst[x * 3 + 1] = t1;
						oDst[x * 3 + 2] = t2;
					}
					else
					{
						ptr += ixs * iCn;
						for (k = 0; k < iCn; k++)
							oDst[x*iCn + k] = ptr[k] * w00 + ptr[k + iCn] * w01 +
							ptr[iSrcStep + k] * w10 + ptr[iSrcStep + k + iCn] * w11;
					}
				}
			}
			else
			{
				for (x = 0; x < iWinSize.width; x++)
				{
					int ixs = cvFloor(xs), iys = cvFloor(ys);
					float a = (float)(xs - ixs), b = (float)(ys - iys), a1 = 1.f - a, b1 = 1.f - b;
					float w00 = a1 * b1, w01 = a * b1, w10 = a1 * b, w11 = a * b;
					const uchar *ptr0, *ptr1;
					xs += A11; ys += A21;

					if ((unsigned)iys < (unsigned)(iSrcSize.height - 1))
						ptr0 = iSrc + iSrcStep * iys, ptr1 = ptr0 + iSrcStep;
					else
						ptr0 = ptr1 = iSrc + (iys < 0 ? 0 : iSrcSize.height - 1)*iSrcStep;

					if ((unsigned)ixs < (unsigned)(iSrcSize.width - 1))
					{
						ptr0 += ixs * iCn; ptr1 += ixs * iCn;
						for (k = 0; k < iCn; k++)
							oDst[x*iCn + k] = ptr0[k] * w00 + ptr0[k + iCn] * w01 + ptr1[k] * w10 + ptr1[k + iCn] * w11;
					}
					else
					{
						ixs = ixs < 0 ? 0 : iSrcSize.width - 1;
						ptr0 += ixs * iCn; ptr1 += ixs * iCn;
						for (k = 0; k < iCn; k++)
							oDst[x*iCn + k] = ptr0[k] * b1 + ptr1[k] * b;
					}
				}
			}
		}
	}

	void get_quadrangle_sub_pix(const cv::Mat& iSrc, cv::Mat& oDst, const cv::Mat& iMap)
	{
    CV_DbgAssert(iSrc.channels() == oDst.channels());

		cv::Size win_size = oDst.size();
		double matrix[6] = { 0 };
		cv::Mat M(2, 3, CV_64F, matrix);
		iMap.convertTo(M, CV_64F);
		double dx = (win_size.width - 1)*0.5;
		double dy = (win_size.height - 1)*0.5;
		matrix[2] -= matrix[0] * dx + matrix[1] * dy;
		matrix[5] -= matrix[3] * dx + matrix[4] * dy;

		if (iSrc.depth() == CV_8U && oDst.depth() == CV_32F)
			get_quadrangle_sub_pix_8u32f(iSrc.ptr(), iSrc.step, iSrc.size(),
				oDst.ptr<float>(), oDst.step, oDst.size(),
				matrix, iSrc.channels());
		else
		{
			CV_DbgAssert(iSrc.depth() == oDst.depth());
			cv::warpAffine(iSrc, oDst, M, oDst.size(),
				cv::INTER_LINEAR + cv::WARP_INVERSE_MAP,
				cv::BORDER_REPLICATE);
		}
	}
}

//=============================================================================
void CalcSimT(cv::Mat &src, cv::Mat &dst,
	double &a, double &b, double &tx, double &ty)
{
  CV_DbgAssert((src.type() == CV_64F) && (dst.type() == CV_64F) &&
		(src.rows == dst.rows) && (src.cols == dst.cols) && (src.cols == 1));
	int i, n = src.rows / 2;
	cv::Mat H(4, 4, CV_64F, cv::Scalar(0));
	cv::Mat g(4, 1, CV_64F, cv::Scalar(0));
	cv::Mat p(4, 1, CV_64F);
	cv::MatIterator_<double> ptr1x = src.begin<double>();
	cv::MatIterator_<double> ptr1y = src.begin<double>() + n;
	cv::MatIterator_<double> ptr2x = dst.begin<double>();
	cv::MatIterator_<double> ptr2y = dst.begin<double>() + n;
	for (i = 0; i < n; i++, ++ptr1x, ++ptr1y, ++ptr2x, ++ptr2y)
	{
		H.db(0, 0) += SQR(*ptr1x) + SQR(*ptr1y);
		H.db(0, 2) += *ptr1x; H.db(0, 3) += *ptr1y;
		g.db(0, 0) += (*ptr1x)*(*ptr2x) + (*ptr1y)*(*ptr2y);
		g.db(1, 0) += (*ptr1x)*(*ptr2y) - (*ptr1y)*(*ptr2x);
		g.db(2, 0) += *ptr2x; g.db(3, 0) += *ptr2y;
	}
	H.db(1, 1) = H.db(0, 0); H.db(1, 2) = H.db(2, 1) = -1.0*(H.db(3, 0) = H.db(0, 3));
	H.db(1, 3) = H.db(3, 1) = H.db(2, 0) = H.db(0, 2); H.db(2, 2) = H.db(3, 3) = n;
	cv::solve(H, g, p, cv::DECOMP_CHOLESKY);
	a = p.db(0, 0); b = p.db(1, 0); tx = p.db(2, 0); ty = p.db(3, 0);
}
//=============================================================================
void invSimT(double a1, double b1, double tx1, double ty1,
	double& a2, double& b2, double& tx2, double& ty2)
{
	cv::Mat M = (cv::Mat_<double>(2, 2) << a1, -b1, b1, a1);
	cv::Mat N = M.inv(cv::DECOMP_SVD); a2 = N.db(0, 0); b2 = N.db(1, 0);
	tx2 = -1.0*(N.db(0, 0)*tx1 + N.db(0, 1)*ty1);
	ty2 = -1.0*(N.db(1, 0)*tx1 + N.db(1, 1)*ty1);
}
//=============================================================================
void SimT(cv::Mat &s, double a, double b, double tx, double ty)
{
  CV_DbgAssert((s.type() == CV_64F) && (s.cols == 1));
	int i, n = s.rows / 2; double x, y;
	cv::MatIterator_<double> xp = s.begin<double>(), yp = s.begin<double>() + n;
	for (i = 0; i < n; i++, ++xp, ++yp)
	{
		x = *xp; y = *yp; *xp = a * x - b * y + tx; *yp = b * x + a * y + ty;
	}
}
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
CLM& CLM::operator= (CLM const& rhs)
{
	if (this != &rhs)
	{
		this->_pdm = rhs._pdm;
		this->_plocal = rhs._plocal.clone();
		this->_pglobl = rhs._pglobl.clone();
		this->_refs = rhs._refs.clone();
		this->_cent.resize(rhs._cent.size());
		this->_visi.resize(rhs._visi.size());
		this->_patch.resize(rhs._patch.size());
		for (int i = 0; i < (int)rhs._cent.size(); i++)
		{
			this->_cent[i] = rhs._cent[i].clone();
			this->_visi[i] = rhs._visi[i].clone();
			this->_patch[i].resize(rhs._patch[i].size());
			for (int j = 0; j < (int)rhs._patch[i].size(); j++)
			{
				if (rhs._patch[i][j]._p.empty() == false)
					this->_patch[i][j] = rhs._patch[i][j];
			}
		}
		this->cshape_ = rhs.cshape_.clone();
		this->bshape_ = rhs.bshape_.clone();
		this->oshape_ = rhs.oshape_.clone();
		this->ms_ = rhs.cshape_.clone();
		this->u_ = rhs.u_.clone();
		this->g_ = rhs.g_.clone();
		this->J_ = rhs.J_.clone();
		this->H_ = rhs.H_.clone();
		this->prob_.resize(rhs.prob_.size());
		this->pmem_.resize(rhs.pmem_.size());
		this->wmem_.resize(rhs.pmem_.size());
		for (int i = 0; i < (int)rhs.prob_.size(); i++)
		{
			this->prob_[i] = rhs.prob_[i].clone();
			this->pmem_[i] = rhs.pmem_[i].clone();
			this->wmem_[i] = rhs.wmem_[i].clone();
		}
	}
	return *this;
}
//=============================================================================
void CLM::Init(PDM &s, cv::Mat &r, vector<cv::Mat> &c,
	vector<cv::Mat> &v, vector<vector<MPatch> > &p)
{
	int n = p.size(); CV_DbgAssert(((int)c.size() == n) && ((int)v.size() == n));
  CV_DbgAssert((r.type() == CV_64F) && (r.rows == 2 * s.nPoints()) && (r.cols == 1));
	for (int i = 0; i < n; i++)
	{
    CV_DbgAssert((int)p[i].size() == s.nPoints());
    CV_DbgAssert((c[i].type() == CV_64F) && (c[i].rows == 3) && (c[i].cols == 1));
    CV_DbgAssert((v[i].type() == CV_32S) && (v[i].rows == s.nPoints()) &&
			(v[i].cols == 1));
	}
	_pdm = s; _refs = r.clone(); _cent.resize(n); _visi.resize(n); _patch.resize(n);
	for (int i = 0; i < n; i++)
	{
		_cent[i] = c[i].clone(); _visi[i] = v[i].clone();
		_patch[i].resize(p[i].size());
		for (int j = 0; j < (int)p[i].size(); j++)_patch[i][j] = p[i][j];
	}
	_plocal.create(_pdm.nModes(), 1, CV_64F);
	_pglobl.create(6, 1, CV_64F);
	cshape_.create(2 * _pdm.nPoints(), 1, CV_64F);
	bshape_.create(2 * _pdm.nPoints(), 1, CV_64F);
	oshape_.create(2 * _pdm.nPoints(), 1, CV_64F);
	ms_.create(2 * _pdm.nPoints(), 1, CV_64F);
	u_.create(6 + _pdm.nModes(), 1, CV_64F);
	g_.create(6 + _pdm.nModes(), 1, CV_64F);
	J_.create(2 * _pdm.nPoints(), 6 + _pdm.nModes(), CV_64F);
	H_.create(6 + _pdm.nModes(), 6 + _pdm.nModes(), CV_64F);
	prob_.resize(_pdm.nPoints()); pmem_.resize(_pdm.nPoints());
	wmem_.resize(_pdm.nPoints());
}
//=============================================================================
void CLM::Load(const char* fname)
{
	ifstream s(fname); CV_DbgAssert(s.is_open()); this->Read(s); s.close();
}
//=============================================================================
void CLM::Save(const char* fname)
{
	ofstream s(fname); CV_DbgAssert(s.is_open()); this->Write(s); s.close();
}
//=============================================================================
void CLM::Write(ofstream &s)
{
	int type = IO::CLM;
	int n = _patch.size();
	s << type << " " << n << " ";
	_pdm.Write(s);
	IO::WriteMat(s, _refs);
	for (int i = 0; i < (int)_cent.size(); i++)IO::WriteMat(s, _cent[i]);
	for (int i = 0; i < (int)_visi.size(); i++)IO::WriteMat(s, _visi[i]);
	for (int i = 0; i < (int)_patch.size(); i++)
	{
		for (int j = 0; j < _pdm.nPoints(); j++)_patch[i][j].Write(s);
	}
}

//=============================================================================
void CLM::Read(ifstream &s, bool readType)
{
	if (readType) { int type; s >> type; CV_DbgAssert(type == IO::CLM); }
	int n; s >> n; _pdm.Read(s); _cent.resize(n); _visi.resize(n);
	_patch.resize(n); IO::ReadMat(s, _refs);
	for (int i = 0; i < (int)_cent.size(); i++)IO::ReadMat(s, _cent[i]);
	for (int i = 0; i < (int)_visi.size(); i++)IO::ReadMat(s, _visi[i]);
	for (int i = 0; i < (int)_patch.size(); i++)
	{
		_patch[i].resize(_pdm.nPoints());
		for (int j = 0; j < _pdm.nPoints(); j++)_patch[i][j].Read(s);
	}
	_plocal.create(_pdm.nModes(), 1, CV_64F);
	_pglobl.create(6, 1, CV_64F);
	cshape_.create(2 * _pdm.nPoints(), 1, CV_64F);
	bshape_.create(2 * _pdm.nPoints(), 1, CV_64F);
	oshape_.create(2 * _pdm.nPoints(), 1, CV_64F);
	ms_.create(2 * _pdm.nPoints(), 1, CV_64F);
	u_.create(6 + _pdm.nModes(), 1, CV_64F);
	g_.create(6 + _pdm.nModes(), 1, CV_64F);
	J_.create(2 * _pdm.nPoints(), 6 + _pdm.nModes(), CV_64F);
	H_.create(6 + _pdm.nModes(), 6 + _pdm.nModes(), CV_64F);
	prob_.resize(_pdm.nPoints()); pmem_.resize(_pdm.nPoints());
	wmem_.resize(_pdm.nPoints());
}

//=============================================================================
int CLM::GetViewIdx()
{
	int idx = 0;
	if (this->nViews() == 1)return 0;
	else
	{
		int i; double v1, v2, v3, d, dbest = -1.0;
		for (i = 0; i < this->nViews(); i++)
		{
			v1 = _pglobl.db(1, 0) - _cent[i].db(0, 0);
			v2 = _pglobl.db(2, 0) - _cent[i].db(1, 0);
			v3 = _pglobl.db(3, 0) - _cent[i].db(2, 0);
			d = v1 * v1 + v2 * v2 + v3 * v3;
			if (dbest < 0 || d < dbest) { dbest = d; idx = i; }
		}

		return idx;
	}
}
//=============================================================================
void CLM::Fit(const cv::Mat& im, vector<int> &wSize,
	int nIter, double clamp, double fTol)
{
	FACE_PROFILER(3_Users_Fit_Shape);

  CV_DbgAssert(im.type() == CV_8UC1);
	int i, idx, n = _pdm.nPoints(); double a1, b1, tx1, ty1, a2, b2, tx2, ty2;
	for (int witer = 0; witer < (int)wSize.size(); witer++)
	{
		_pdm.CalcShape2D(cshape_, _plocal, _pglobl);
		CalcSimT(_refs, cshape_, a1, b1, tx1, ty1);
		invSimT(a1, b1, tx1, ty1, a2, b2, tx2, ty2);
		idx = this->GetViewIdx();
#ifdef _OPENMP
#pragma omp parallel for
#endif
		for (i = 0; i < n; i++)
		{
			if (_visi[idx].rows == n && _visi[idx].it(i, 0) == 0) { continue; }
			int w = wSize[witer] + _patch[idx][i]._w - 1;
			int h = wSize[witer] + _patch[idx][i]._h - 1;
			cv::Mat sim =
				(cv::Mat_<float>(2, 3) << a1, -b1, cshape_.db(i, 0), b1, a1, cshape_.db(i + n, 0));
			if ((w > wmem_[i].cols) || (h > wmem_[i].rows))wmem_[i].create(h, w, CV_32F);
			cv::Mat wimg = wmem_[i](cv::Rect(0, 0, w, h));
			get_quadrangle_sub_pix(im, wimg, sim);
			if (wSize[witer] > pmem_[i].rows)
				pmem_[i].create(wSize[witer], wSize[witer], CV_64F);
			prob_[i] = pmem_[i](cv::Rect(0, 0, wSize[witer], wSize[witer]));
			_patch[idx][i].Response(wimg, prob_[i]);
		}
		SimT(cshape_, a2, b2, tx2, ty2);
		_pdm.ApplySimT(a2, b2, tx2, ty2, _pglobl);
		cshape_.copyTo(bshape_);
		this->Optimize(idx, wSize[witer], nIter, fTol, clamp, 1);
		this->Optimize(idx, wSize[witer], nIter, fTol, clamp, 0);
		_pdm.ApplySimT(a1, b1, tx1, ty1, _pglobl);
	}
}
//=============================================================================
void CLM::Optimize(int idx, int wSize, int nIter,
	double fTol, double clamp, bool rigid)
{
	int i, m = _pdm.nModes(), n = _pdm.nPoints();
	double var, sigma = (wSize*wSize) / 36.0; cv::Mat u, g, J, H;
	if (rigid)
	{
		u = u_(cv::Rect(0, 0, 1, 6));   g = g_(cv::Rect(0, 0, 1, 6));
		J = J_(cv::Rect(0, 0, 6, 2 * n)); H = H_(cv::Rect(0, 0, 6, 6));
	}
	else { u = u_; g = g_; J = J_; H = H_; }
	for (int iter = 0; iter < nIter; iter++)
	{
		_pdm.CalcShape2D(cshape_, _plocal, _pglobl);
		if (iter > 0 && cv::norm(cshape_, oshape_) < fTol) { break; }
		cshape_.copyTo(oshape_);
		if (rigid)_pdm.CalcRigidJacob(_plocal, _pglobl, J);
		else     _pdm.CalcJacob(_plocal, _pglobl, J);
#ifdef _OPENMP
#pragma omp parallel for
#endif
		for (i = 0; i < n; i++)
		{
			if (_visi[idx].rows == n && _visi[idx].it(i, 0) == 0)
			{
				cv::Mat Jx = J.row(i); Jx = cv::Scalar::all(0);
				cv::Mat Jy = J.row(i + n); Jy = cv::Scalar::all(0);
				ms_.db(i, 0) = 0.0; ms_.db(i + n, 0) = 0.0; continue;
			}
			double dx = cshape_.db(i, 0) - bshape_.db(i, 0) + (wSize - 1) / 2;
			double dy = cshape_.db(i + n, 0) - bshape_.db(i + n, 0) + (wSize - 1) / 2;
			int ii, jj; double v, vx, vy, mx = 0.0, my = 0.0, sum = 0.0;
			cv::MatIterator_<double> p = prob_[i].begin<double>();
			for (ii = 0; ii < wSize; ii++)
			{
				vx = (dy - ii)*(dy - ii);
				for (jj = 0; jj < wSize; jj++)
				{
					vy = (dx - jj)*(dx - jj);
					v = *p++; v *= exp(-0.5*(vx + vy) / sigma);
					sum += v;  mx += v * jj;  my += v * ii;
				}
			}
			ms_.db(i, 0) = mx / sum - dx; ms_.db(i + n, 0) = my / sum - dy;
		}
		g = J.t()*ms_; H = J.t()*J;
		if (!rigid)
		{
			for (i = 0; i < m; i++)
			{
				var = 0.5*sigma / _pdm._E.db(0, i);
				H.db(6 + i, 6 + i) += var; g.db(6 + i, 0) -= var * _plocal.db(i, 0);
			}
		}
		u_ = cv::Scalar::all(0); cv::solve(H, g, u, cv::DECOMP_CHOLESKY);
		_pdm.CalcReferenceUpdate(u_, _plocal, _pglobl);
		if (!rigid)_pdm.Clamp(_plocal, clamp);
	}
}
//=============================================================================
