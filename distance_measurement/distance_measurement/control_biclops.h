#ifndef _CONTROL_BICLOPS_H_
#define _CONTROL_BICLOPS_H_

//�yBiclops�֌W�z
#include "Biclops/include/Biclops.h"
#include "Biclops/include/PMDUtils.h"
#include "Biclops/include/PMDAxisControl.h"
//#if _DEBUG
//#pragma comment (lib, "Biclops/lib/libBiclopsD.lib")
//#pragma comment (lib, "Biclops/lib/libPMDD.lib")
//#pragma comment (lib, "Biclops/lib/libUtilsD.lib")
//#else
//#pragma comment (lib, "Biclops/lib/libBiclops.lib")
//#pragma comment (lib, "Biclops/lib/libPMD.lib")
//#pragma comment (lib, "Biclops/lib/libUtils.lib")
//#endif
#pragma comment (lib, "pthread/lib/pthreadVC.lib")
#pragma comment (lib, "pthread/lib/pthreadVCE.lib")
#pragma comment (lib, "pthread/lib/pthreadVSE.lib")

#include <time.h>

//�yBiclops�֘A�̕ϐ��z
const int axisMask = Biclops::PanMask + Biclops::TiltMask;//��]���̎w��

#define CONTROL_BICLOPS_IMAGE_WIDTH 640
#define CONTROL_BICLOPS_IMAGE_HEIGHT 480

class ControlBiclops /*�_��Fcamera platform*/
{
public:
	//Biclops�֘A�̕ϐ��E�֐�
	Biclops biclops;
	PMDAxisControl *AxisParam;
	PMDAxisControl *panAxis;    //������p�̃|�C���^�ϐ�
	PMDAxisControl *tiltAxis;   //������p�̃|�C���^�ϐ�
	PMDAxisControl::Profile profile;//�����i�[�ϐ�
	PMDint32 pan_pos, tilt_pos, past_pan, past_tilt; //��]��̌��݈ʒu(�J�E���g���j
	PMDint32 pan_pos_init;	//��]��̏������̍ۂ̈ʒu
	double pan_angle_rad, tilt_angle_rad;
	void  deviceTurn(double pan, double tilt); //�J�����̉�]�Ɏg�p����֐�

	//�R���X�g���N�^
	ControlBiclops(const char *p);	//p�ɂ�Biclops�̐ݒ�t�@�C���̃p�X������
	//�f�R���X�g���N�^
	~ControlBiclops();

	void getPosition();					//��]��p�x�擾���\�b�h
	void turnHome();				//�z�[���|�W�V�����ɖ߂郁�\�b�h
	void directionTurn(double X, double Y);	//��]��̕�����ς���֐�


};


#endif _CONTROL_BICLOPS_H_