#include "control_biclops.h"
#define _USE_MATH_DEFINES
#include <math.h>

//---------------------------------------------------------------------------
//ControlBiclops�N���X�֐�///////////////////////////////////////////////////
//---------------------------------------------------------------------------
//�R���X�g���N�^/////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
ControlBiclops::ControlBiclops(const char *p){
	//biclops�̏�����
	this->biclops.Initialize(p);
	this->panAxis = biclops.GetAxis(Biclops::Pan);
	this->tiltAxis = biclops.GetAxis(Biclops::Tilt);
	this->biclops.HomeAxes(axisMask);
	//���݈ʒu�̎擾
	this->panAxis->GetActualPosition(this->pan_pos_init);
	this->panAxis->GetActualPosition(pan_pos);
	this->tiltAxis->GetActualPosition(tilt_pos);
}

//---------------------------------------------------------------------------
//�f�R���X�g���N�^///////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
ControlBiclops::~ControlBiclops(){
}

//---------------------------------------------------------------------------
//��]��̌��݈ʒu���擾����֐�/////////////////////////////////////////////
//---------------------------------------------------------------------------
void ControlBiclops::getPosition()
{
	//���݈ʒu�̎擾
	past_pan = pan_pos;	//�O��ʒu�̕ۑ�
	past_tilt = tilt_pos;	//�O��ʒu�̕ۑ�
	panAxis->GetActualPosition(pan_pos);
	tiltAxis->GetActualPosition(tilt_pos);
}

//---------------------------------------------------------------------------
//��]����z�[���|�W�V�����ɖ߂�/////////////////////////////////////////////
//---------------------------------------------------------------------------
void ControlBiclops::turnHome()
{
	panAxis->GetProfile(profile);
	profile.pos = 0;
	panAxis->SetProfile(profile);

	tiltAxis->GetProfile(profile);
	profile.pos = 0;
	tiltAxis->SetProfile(profile);

	biclops.Move(axisMask, 0);
}

//---------------------------------------------------------------------------
//��]��̕�����ς���֐�///////////////////////////////////////////////////
//---------------------------------------------------------------------------
void ControlBiclops::directionTurn(double X, double Y)
{
	panAxis->GetProfile(profile);
	profile.pos += X / CONTROL_BICLOPS_IMAGE_WIDTH / 64;
	panAxis->SetProfile(profile);

	tiltAxis->GetProfile(profile);
	profile.pos += Y / CONTROL_BICLOPS_IMAGE_HEIGHT / 64;
	tiltAxis->SetProfile(profile);

	biclops.Move(axisMask, 0);
}

//---------------------------------------------------------------------------
//�J�����̉�]�Ɏg�p����֐�/////////////////////////////////////////////////
//---------------------------------------------------------------------------
void ControlBiclops::deviceTurn(double pan_angle, double tilt_angle)
{
	////��������̕ύX
	////��Ɉړ�������
	time_t start, end;
	panAxis->GetProfile(profile);//0ms
	if (fabs(profile.pos) < 0.4  /*&& abs(dx) > 5*/)//�O�̏������p�x�ۏ�(revs)�C���̏����������ړ��̐����i�s�N�Z���j
	{
		profile.pos = PMDUtils::RadsToRevs(pan_angle) + PMDUtils::DegsToRevs(pan_pos / 33.33);
		profile.vel = 0.4;  //��]��̑��x�̕ύX
		profile.acc = 0.1;  //��]��̉����x�̕ύX
		panAxis->SetProfile(profile);
	}
	tiltAxis->GetProfile(profile);
	if (fabs(profile.pos) < 0.15 && abs(tilt_angle) > 0.15*M_PI / 180. /*&& abs(dy) > 5*/)
	{
		profile.pos = PMDUtils::RadsToRevs(tilt_angle) + PMDUtils::DegsToRevs(tilt_pos / 33.33);//0ms
		profile.vel = 0.4;  //��]��̑��x�̕ύX
		profile.acc = 0.1;  //��]��̉����x�̕ύX
		tiltAxis->SetProfile(profile); //16ms
	}
	start = clock();
	biclops.Move(axisMask, 0);//48ms
	end = clock();
	//printf("%d[ms]\n",end-start);
}

