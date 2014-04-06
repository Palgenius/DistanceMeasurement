#include "vcc.h"

VCC::VCC(int thres){
	char VCC_CorrelationMapB[256] = { 
		0, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3,
		1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3,
		1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4,
		2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4,
		1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4,
		2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4,
		1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4,
		2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4 
	};
	char* MapG = this->VCC_CorrelationMap;
	char* MapA = VCC_CorrelationMapB;
	for (short a = 256; a; a--, MapA++){
		char* MapB = VCC_CorrelationMapB;
		for (short b = 256; b; b--, MapB++, MapG++){
			*MapG = -(*MapA + *MapB);
		}
	}
	this->threshold = thres;
	for (int j = 0; j < VCC_DATABASE_SIZE; j++){
		std::vector<cv::Mat> templateImage_x;
		for (int i = 0; i < VCC_DATABASE_SIZE; i++){
			char filename[256];
			sprintf_s(filename, VCC_TEMPLATE_DATABASE_PATH, j*VCC_DATABASE_SIZE + i);
			cv::Mat templateReadImage = cv::imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
			templateImage_x.push_back(templateReadImage);
			cv::Mat templateResizeImage(VCC_TEMPLATE_SIZE + 2, VCC_TEMPLATE_SIZE + 2, CV_8UC1);
			cv::resize(templateReadImage, templateResizeImage,templateResizeImage.size());
			for (int y = 0; y < VCC_TEMPLATE_SIZE + 2; y++){
				for (int x = 0; x < VCC_TEMPLATE_SIZE + 2; x++){
					this->templateImageData[j][i][y*(VCC_TEMPLATE_SIZE + 2) + x]
						= (unsigned char)*(templateResizeImage.data + y*templateResizeImage.cols + x);
				}
			}
		}
		this->templateImage.push_back(templateImage_x);
	}
	for (int i = 0; i<VCC_DATABASE_SIZE; i++){
		for (int j = 0; j<VCC_DATABASE_SIZE; j++){

			unsigned char* input = this->templateImageData[i][j] + VCC_TEMPLATE_SIZE + 2 + 1;//�e���v���[�g�摜�f�[�^�̐擪�A�h���X+1�񕪂̃s�N�Z��+1�s�N�Z��(VCC_TEMPLATE_SIZE+2)+1
			unsigned short* templateAddress = this->templateImageVectorCode[i][j];	//�e���v���[�gVCC�摜�̐擪�A�h���X
			for (int z = VCC_TEMPLATE_SIZE; z; z--, input += (VCC_TEMPLATE_SIZE + 2) - VCC_TEMPLATE_SIZE){
				for (int y = VCC_TEMPLATE_QUARTER_SIZE; y; y--, templateAddress++){
					unsigned short VC = 0;
					for (int x = 4; x; x--, input++){
						VC <<= 4;
						int VCX = *(input + (VCC_TEMPLATE_SIZE + 2) + 1) + *(input + 1) + *(input - (VCC_TEMPLATE_SIZE + 2) + 1) - *(input + (VCC_TEMPLATE_SIZE + 2) - 1) - *(input - 1) - *(input - (VCC_TEMPLATE_SIZE + 2) - 1);
						int VCY = *(input + (VCC_TEMPLATE_SIZE + 2) + 1) + *(input + (VCC_TEMPLATE_SIZE + 2)) + *(input + (VCC_TEMPLATE_SIZE + 2) - 1) - *(input - (VCC_TEMPLATE_SIZE + 2) + 1) - *(input - (VCC_TEMPLATE_SIZE + 2)) - *(input - (VCC_TEMPLATE_SIZE + 2) - 1);
						if (VCX>VCC_VC_THRESHOLD)VC += 1; else if (VCX<-VCC_VC_THRESHOLD)VC += 2;
						if (VCY>VCC_VC_THRESHOLD)VC += 4; else if (VCY<-VCC_VC_THRESHOLD)VC += 8;
					}
					*templateAddress = VC;
				}
			}
			//this->MkTmpBL = false;
		}
	}

	//kalmanfilter

	this->KF = new cv::KalmanFilter(4, 2, 0);
	this->state = new cv::Mat_<float>(4, 1); /* (x, y, Vx, Vy) */
	this->processNoise = new cv::Mat(4, 1, CV_32F);
	this->measurement = new cv::Mat_<float>(2, 1);
	this->measurement->setTo(cv::Scalar(0));


	this->targetDB_x = 4;
	this->targetDB_y = 4;
	this->databaseFlag = true;
	this->subpixelFlag = true;
	this->kalmanFlag = true;
	this->allSeekFlag = true;
	this->databaseClearFlag = false;
	this->databaseAllSearchFlag = true;
	this->allSeekThreshold = 800;
	this->databaseSearchThreshold = 800;
}

VCC::~VCC()
{
}

void VCC::kalmanInitialize(double get_x, double get_y){

	KF->statePre.at<float>(0) = get_x;
	KF->statePre.at<float>(1) = get_y;
	KF->statePre.at<float>(2) = 0;
	KF->statePre.at<float>(3) = 0;
	KF->transitionMatrix = *(cv::Mat_<float>(4, 4) << 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

	setIdentity(KF->measurementMatrix);
	setIdentity(KF->processNoiseCov, cv::Scalar::all(1e-3));
	setIdentity(KF->measurementNoiseCov, cv::Scalar::all(1e-1));
	setIdentity(KF->errorCovPost, cv::Scalar::all(.1));

}

void VCC::setInputImage(cv::Mat image){
	unsigned char *imagePointer = image.data;
	for (int i = 0; i < VCC_INPUT_IMAGE_SIZE_X*VCC_INPUT_IMAGE_SIZE_Y; i++){
		this->inputImage[i] = *(imagePointer++);
	}
}

void VCC::templateMatching(){
	unsigned char* input;//Inpt ���͉摜�A�N�Z�X�|�C���^
	unsigned short* vectorCode;//VcCd ���̓x�N�g�������摜�A�N�Z�X�|�C���^
	unsigned short* matchingPosition;//McPs �}�b�`���O�ӏ�
	unsigned short inputVectorCodeSeekArea[VCC_SEEK_AREA_SIZE_X*VCC_SEEK_AREA_SIZE];//VcCdImg �̈�T���p�}�b�`���O�i�[�z��
	unsigned short inputVectorCodeAllArea[(VCC_INPUT_IMAGE_SIZE_X - 5)*(VCC_INPUT_IMAGE_SIZE_Y - 2)];//McSt �S�T���p�}�b�`���O�i�[�z��
	unsigned short* matchingCoordinate;//McXY �}�b�`���O���W�l�ۑ��|�C���^
	char* correlationMap = VCC_CorrelationMap;//CrrMp ���֒l�v�Z�p�e�[�u���A�N�Z�X�|�C���^
	int* matchingParameterPointer = this->matchingParameters;//MPrm �p�����[�^�A�N�Z�X�|�C���^
	int correlationMinimam = 2048;//CrrMin ���֒l�̍ŏ�


	/*********************�������l�ȉ��̏ꍇ(�̈�T��)*******************************/
	if (*(matchingParameterPointer + 8) < this->allSeekThreshold || !this->allSeekFlag){
		/*---------------------�x�N�g��������-------------------------------------------*/
		input = inputImage + (*(matchingParameterPointer + 6) - VCC_SEEK_AREA_HALF_SIZE) + (*(matchingParameterPointer + 7) - VCC_SEEK_AREA_HALF_SIZE)*VCC_INPUT_IMAGE_SIZE_X;	//�T���̈�̐擪�A�h���X�̓���
		vectorCode = inputVectorCodeSeekArea;
		for (int y = VCC_SEEK_AREA_SIZE; y; y--, input += VCC_INPUT_IMAGE_SIZE_X - VCC_SEEK_AREA_SIZE)
		{
			unsigned short VC = 0;
			for (int x = 3; x; x--, input++)
			{
				VC <<= 4;
				int VCX = *(input + VCC_INPUT_IMAGE_SIZE_X + 1) + *(input + 1) + *(input - VCC_INPUT_IMAGE_SIZE_X + 1) - *(input + VCC_INPUT_IMAGE_SIZE_X - 1) - *(input - 1) - *(input - VCC_INPUT_IMAGE_SIZE_X - 1);
				int VCY = *(input + VCC_INPUT_IMAGE_SIZE_X + 1) + *(input + VCC_INPUT_IMAGE_SIZE_X) + *(input + VCC_INPUT_IMAGE_SIZE_X - 1) - *(input - VCC_INPUT_IMAGE_SIZE_X + 1) - *(input - VCC_INPUT_IMAGE_SIZE_X) - *(input - VCC_INPUT_IMAGE_SIZE_X - 1);
				if (VCX > VCC_VECTOR_CODE_THRESHOLD)VC += 1; else if (VCX < -VCC_VECTOR_CODE_THRESHOLD)VC += 2;
				if (VCY > VCC_VECTOR_CODE_THRESHOLD)VC += 4; else if (VCY < -VCC_VECTOR_CODE_THRESHOLD)VC += 8;
			}
			for (int x = VCC_SEEK_AREA_SIZE_X; x; x--, input++, vectorCode++)
			{
				VC <<= 4;
				int VCX = *(input + VCC_INPUT_IMAGE_SIZE_X + 1) + *(input + 1) + *(input - VCC_INPUT_IMAGE_SIZE_X + 1) - *(input + VCC_INPUT_IMAGE_SIZE_X - 1) - *(input - 1) - *(input - VCC_INPUT_IMAGE_SIZE_X - 1);
				int VCY = *(input + VCC_INPUT_IMAGE_SIZE_X + 1) + *(input + VCC_INPUT_IMAGE_SIZE_X) + *(input + VCC_INPUT_IMAGE_SIZE_X - 1) - *(input - VCC_INPUT_IMAGE_SIZE_X + 1) - *(input - VCC_INPUT_IMAGE_SIZE_X) - *(input - VCC_INPUT_IMAGE_SIZE_X - 1);
				if (VCX>VCC_VECTOR_CODE_THRESHOLD)VC += 1; else if (VCX<-VCC_VECTOR_CODE_THRESHOLD)VC += 2;
				if (VCY>VCC_VECTOR_CODE_THRESHOLD)VC += 4; else if (VCY < -VCC_VECTOR_CODE_THRESHOLD)VC += 8;
				*vectorCode = VC;
			}
		}
		/*---------------------���֌v�Z-------------------------------------------------*/
		matchingPosition = inputVectorCodeSeekArea;
		for (int yy = VCC_SEEK_AREA_VECTOR_CODE_SIZE; yy; yy--, matchingPosition += VCC_SEEK_AREA_SIZE_X - VCC_SEEK_AREA_VECTOR_CODE_SIZE){
			for (int xx = VCC_SEEK_AREA_VECTOR_CODE_SIZE; xx; xx--, matchingPosition++){
				unsigned short* templateVC;//TmpB

				templateVC = this->templateImageVectorCode[this->targetDB_x][this->targetDB_y];

				int correlationScore = correlationMinimam;
				vectorCode = matchingPosition;
				for (int y = VCC_TEMPLATE_SIZE/4; y&&correlationScore > 0; y--, vectorCode += VCC_SEEK_AREA_SIZE_X - VCC_TEMPLATE_SIZE)
				for (int x = VCC_TEMPLATE_QUARTER_SIZE; x; x--, vectorCode += 4, templateVC++)
					correlationScore += *(correlationMap + (*vectorCode^*templateVC));
				if (correlationScore <= 0)continue;
				correlationMinimam -= correlationScore;
				matchingCoordinate = matchingPosition;
			}
		}

		/*---------------------DB�T���i����x���傫����������s�j---�K�v�Ȃ��ꍇ�͂������R�����g�A�E�g----*/
		if (this->databaseClearFlag){
			this->targetDB_x = 4;
			this->targetDB_y = 4;
			this->databaseClearFlag = false;
		}
		if ((correlationMinimam >= this->databaseSearchThreshold) && this->databaseFlag || this->databaseAllSearchFlag){
			//�e���v���[�g�f�[�^�x�[�X�T���̍ۂ̈ꎞ�ۊǗp�ϐ�
			int tempCorrelationMinimum;//tmpCrrMin
			int targetDB_x_min, targetDB_y_min;

			if (this->databaseAllSearchFlag){
				for (int j = this->targetDB_x - 8; j <= this->targetDB_x + 8; j++){
					for (int i = this->targetDB_y - 8; i <= this->targetDB_y + 8; i++){
						if (j < 0 || VCC_DATABASE_SIZE - 1 < j) continue;	//�f�[�^�x�[�X�͈̔͊O��T�����Ȃ�
						if (i < 0 || VCC_DATABASE_SIZE - 1 < i) continue;

						unsigned short* tempMatchingPosition = inputVectorCodeSeekArea;
						unsigned short* tempMatchingCoordinate;
						tempCorrelationMinimum = 2048;

						/*���֌v�Z----------------------------------------------------------*/
						for (int yy = VCC_SEEK_AREA_VECTOR_CODE_SIZE; yy; yy--, tempMatchingPosition += VCC_SEEK_AREA_SIZE_X - VCC_SEEK_AREA_VECTOR_CODE_SIZE){
							for (int xx = VCC_SEEK_AREA_VECTOR_CODE_SIZE; xx; xx--, tempMatchingPosition++){
								unsigned short* templateVC;//TmpB
								templateVC = this->templateImageVectorCode[j][i];

								int correlationScore = tempCorrelationMinimum;
								vectorCode = tempMatchingPosition;
								for (int y = VCC_TEMPLATE_SIZE; y&&correlationScore > 0; y--, vectorCode += VCC_SEEK_AREA_SIZE_X - VCC_TEMPLATE_SIZE)
								for (int x = VCC_TEMPLATE_QUARTER_SIZE; x; x--, vectorCode += 4, templateVC++)
									correlationScore += *(correlationMap + (*vectorCode^*templateVC));
								if (correlationScore <= 0)continue;
								tempCorrelationMinimum -= correlationScore;
								tempMatchingCoordinate = tempMatchingPosition;
							}
						}
						//printf("[%d,%d][correlationMinimam=%d]\n", j,i,tempCorrelationMinimum);
						/*���֒l�v�Z�����܂�------------------------------------------------------*/
						if (tempCorrelationMinimum < correlationMinimam){
							correlationMinimam = tempCorrelationMinimum;
							matchingCoordinate = tempMatchingCoordinate;
							targetDB_x_min = j;
							targetDB_y_min = i;
							this->targetDB_x = targetDB_x_min;
							this->targetDB_y = targetDB_y_min;
						}
					}//��̒T��
				}//�s�̒T��
				this->databaseAllSearchFlag = false;
			}
			else{
				for (int j = this->targetDB_x - 1; j <= this->targetDB_x + 1; j++){
					for (int i = this->targetDB_y - 1; i <= this->targetDB_y + 1; i++){
						if (j < 0 || VCC_DATABASE_SIZE - 1 < j) continue;	//�f�[�^�x�[�X�͈̔͊O��T�����Ȃ�
						if (i < 0 || VCC_DATABASE_SIZE - 1 < i) continue;
						//if (!(j == this->targetDB_x || i == this->targetDB_y)) continue;


						unsigned short* tempMatchingPosition = inputVectorCodeSeekArea;
						unsigned short* tempMatchingCoordinate;
						tempCorrelationMinimum = 2048;

						/*���֌v�Z----------------------------------------------------------*/
						tempMatchingPosition += (matchingCoordinate - inputVectorCodeSeekArea - 9 - 124 * 9);
						for (int yy = 50; yy; yy--, tempMatchingPosition += 74){
							for (int xx = 50; xx; xx--, tempMatchingPosition++){
								unsigned short* templateVC;//TmpB
								templateVC = this->templateImageVectorCode[j][i];

								int correlationScore = tempCorrelationMinimum;
								vectorCode = tempMatchingPosition;
								for (int y = VCC_TEMPLATE_SIZE/4; y&&correlationScore > 0; y--, vectorCode += VCC_SEEK_AREA_SIZE_X - VCC_TEMPLATE_SIZE)
								for (int x = VCC_TEMPLATE_QUARTER_SIZE; x; x--, vectorCode += 4, templateVC++)
									correlationScore += *(correlationMap + (*vectorCode^*templateVC));
								if (correlationScore <= 0)continue;
								tempCorrelationMinimum -= correlationScore;
								tempMatchingCoordinate = tempMatchingPosition;
							}
						}
						//printf("[%d,%d][correlationMinimam=%d]\n", j,i,tempCorrelationMinimum);
						/*���֒l�v�Z�����܂�------------------------------------------------------*/
						if (tempCorrelationMinimum < correlationMinimam){
							correlationMinimam = tempCorrelationMinimum;
							matchingCoordinate = tempMatchingCoordinate;
							targetDB_x_min = j;
							targetDB_y_min = i;
							this->targetDB_x = targetDB_x_min;
							this->targetDB_y = targetDB_y_min;
						}
					}//��̒T��
				}//�s�̒T��
			}
		}
		/*--------------------------------------------------------------------DB�T�������܂�--------------*/
		/*---------------------�T�u�s�N�Z������@------------------------------------------*/
		if (this->subpixelFlag){
			int Crrpm1[4] = { VCC_CORRELATION_THRESHOLD, VCC_CORRELATION_THRESHOLD, VCC_CORRELATION_THRESHOLD, VCC_CORRELATION_THRESHOLD };
			for (int i = 0; i < 4; i++)
			{
				//X+1�̑���x
				//WORD* TmpB=TmpBImg;
				unsigned short* templateVC;
				templateVC = this->templateImageVectorCode[this->targetDB_x][this->targetDB_y];

				int correlationScore = VCC_CORRELATION_THRESHOLD;
				switch (i){
				case 0: vectorCode = matchingCoordinate + 1; break;    //X+1�̃A�h���X
				case 1: vectorCode = matchingCoordinate - 1; break;    //X-1�̃A�h���X
				case 2: vectorCode = matchingCoordinate + VCC_SEEK_AREA_SIZE_X; break;//Y+1�̃A�h���X
				case 3: vectorCode = matchingCoordinate - VCC_SEEK_AREA_SIZE_X; break;//Y-1�̃A�h���X
				}

				for (int y = VCC_TEMPLATE_SIZE; y&&correlationScore > 0; y--, vectorCode += VCC_SEEK_AREA_SIZE_X - VCC_TEMPLATE_SIZE)
				for (int x = VCC_TEMPLATE_QUARTER_SIZE; x; x--, vectorCode += 4, templateVC++)
					correlationScore += *(correlationMap + (*vectorCode^*templateVC));
				Crrpm1[i] -= correlationScore;
			}

			subpixelResult_x = (double)(Crrpm1[1] - Crrpm1[0]) / (Crrpm1[0] + Crrpm1[1] - 2 * correlationMinimam) / 2.0;
			subpixelResult_y = (double)(Crrpm1[3] - Crrpm1[2]) / (Crrpm1[2] + Crrpm1[3] - 2 * correlationMinimam) / 2.0;
		}
		/*--------------------�J���}���t�B���^����-------------------------------------*/

		cv::Mat prediction = this->KF->predict();
		cv::Point predictPt(prediction.at<float>(0), prediction.at<float>(1));

		(*measurement)(0) = ((int)matchingCoordinate - (int)inputVectorCodeSeekArea) / 2 % VCC_SEEK_AREA_SIZE_X + *(matchingParameterPointer + 6) - VCC_SEEK_AREA_VECTOR_CODE_HALF_SIZE + this->subpixelResult_x;
		(*measurement)(1) = ((int)matchingCoordinate - (int)inputVectorCodeSeekArea) / 2 / VCC_SEEK_AREA_SIZE_X + *(matchingParameterPointer + 7) - VCC_SEEK_AREA_VECTOR_CODE_HALF_SIZE + this->subpixelResult_y;

		cv::Mat estimated = KF->correct((*measurement));
		kalmanx = estimated.at<float>(0);
		kalmany = estimated.at<float>(1);

		/*---------------------���o���ʂ̌v�Z����---------------------------------------*/
		//�O��̌��o�ʒu
		* matchingParameterPointer = *(matchingParameterPointer + 2);
		*(matchingParameterPointer + 1) = *(matchingParameterPointer + 3);

		if (this->kalmanFlag){
			////����̌��o�ʒu�i���S���W�j�J���}���t�B���^�[����
			*(matchingParameterPointer + 2) = kalmanx;
			*(matchingParameterPointer + 3) = kalmany;
		}
		else{
			//����̌��o�ʒu�i���S���W�j
			*(matchingParameterPointer + 2) = ((int)matchingCoordinate - (int)inputVectorCodeSeekArea) / 2 % VCC_SEEK_AREA_SIZE_X + *(matchingParameterPointer + 6) - VCC_SEEK_AREA_VECTOR_CODE_HALF_SIZE + this->subpixelResult_x;
			*(matchingParameterPointer + 3) = ((int)matchingCoordinate - (int)inputVectorCodeSeekArea) / 2 / VCC_SEEK_AREA_SIZE_X + *(matchingParameterPointer + 7) - VCC_SEEK_AREA_VECTOR_CODE_HALF_SIZE + this->subpixelResult_y;
		}

		//�Ώۂ̗\���ʒu(���S���W�j
		*(matchingParameterPointer + 4) = (std::max)((std::min)(*(matchingParameterPointer + 2), VCC_INPUT_IMAGE_SIZE_X - VCC_TEMPLATE_HALF_X), VCC_TEMPLATE_HALF_X);
		*(matchingParameterPointer + 5) = (std::max)((std::min)(*(matchingParameterPointer + 3), VCC_INPUT_IMAGE_SIZE_Y - VCC_TEMPLATE_HALF_X), VCC_TEMPLATE_HALF_X);

		//�����̈�̗\���ʒu(���S���W)
		*(matchingParameterPointer + 6) = (std::max)((std::min)(*(matchingParameterPointer + 2), VCC_INPUT_IMAGE_SIZE_X - VCC_SEEK_AREA_HALF_SIZE), VCC_SEEK_AREA_HALF_SIZE + 1);
		*(matchingParameterPointer + 7) = (std::max)((std::min)(*(matchingParameterPointer + 3), VCC_INPUT_IMAGE_SIZE_Y - VCC_SEEK_AREA_HALF_SIZE), VCC_SEEK_AREA_HALF_SIZE + 1);

		//���֒l(���S���W)
		*(matchingParameterPointer + 8) = correlationMinimam;
	}



	/**********************�������l�𒴂����ꍇ(�S�T��)************************************/
	else{
		/*---------------------�x�N�g��������-------------------------------------------*/
		input = inputImage + VCC_INPUT_IMAGE_SIZE_X + 1;//+(*(matchingParameterPointer+6)-VCC_SEEK_AREA_HALF_SIZE)+(*(matchingParameterPointer+7)-VCC_SEEK_AREA_HALF_SIZE)*VCC_INPUT_IMAGE_SIZE_X;	//�T���̈�̐擪�A�h���X�̓���
		vectorCode = inputVectorCodeAllArea;
		for (int y = VCC_INPUT_IMAGE_SIZE_Y - 2; y; y--, input += 2)
		{
			unsigned short VC = 0;
			for (int x = 3; x; x--, input++)
			{
				VC <<= 4;
				int VCX = *(input + VCC_INPUT_IMAGE_SIZE_X + 1) + *(input + 1) + *(input - VCC_INPUT_IMAGE_SIZE_X + 1) - *(input + VCC_INPUT_IMAGE_SIZE_X - 1) - *(input - 1) - *(input - VCC_INPUT_IMAGE_SIZE_X - 1);
				int VCY = *(input + VCC_INPUT_IMAGE_SIZE_X + 1) + *(input + VCC_INPUT_IMAGE_SIZE_X) + *(input + VCC_INPUT_IMAGE_SIZE_X - 1) - *(input - VCC_INPUT_IMAGE_SIZE_X + 1) - *(input - VCC_INPUT_IMAGE_SIZE_X) - *(input - VCC_INPUT_IMAGE_SIZE_X - 1);
				if (VCX > VCC_VECTOR_CODE_THRESHOLD)VC += 1; else if (VCX<-VCC_VECTOR_CODE_THRESHOLD)VC += 2;
				if (VCY>VCC_VECTOR_CODE_THRESHOLD)VC += 4; else if (VCY<-VCC_VECTOR_CODE_THRESHOLD)VC += 8;
			}
			for (int x = VCC_INPUT_IMAGE_SIZE_X - 5; x; x--, input++, vectorCode++)
			{
				VC <<= 4;
				int VCX = *(input + VCC_INPUT_IMAGE_SIZE_X + 1) + *(input + 1) + *(input - VCC_INPUT_IMAGE_SIZE_X + 1) - *(input + VCC_INPUT_IMAGE_SIZE_X - 1) - *(input - 1) - *(input - VCC_INPUT_IMAGE_SIZE_X - 1);
				int VCY = *(input + VCC_INPUT_IMAGE_SIZE_X + 1) + *(input + VCC_INPUT_IMAGE_SIZE_X) + *(input + VCC_INPUT_IMAGE_SIZE_X - 1) - *(input - VCC_INPUT_IMAGE_SIZE_X + 1) - *(input - VCC_INPUT_IMAGE_SIZE_X) - *(input - VCC_INPUT_IMAGE_SIZE_X - 1);
				if (VCX>VCC_VECTOR_CODE_THRESHOLD)VC += 1; else if (VCX<-VCC_VECTOR_CODE_THRESHOLD)VC += 2;
				if (VCY>VCC_VECTOR_CODE_THRESHOLD)VC += 4; else if (VCY < -VCC_VECTOR_CODE_THRESHOLD)VC += 8;
				*vectorCode = VC;
			}
		}
		/*---------------------���֌v�Z-------------------------------------------------*/
		matchingPosition = inputVectorCodeAllArea;
		for (int yy = VCC_INPUT_IMAGE_SIZE_Y - 2 - VCC_TEMPLATE_SIZE / 4 + 1; yy; yy--, matchingPosition += VCC_TEMPLATE_SIZE - 4){
			for (int xx = VCC_INPUT_IMAGE_SIZE_X - 2 - VCC_TEMPLATE_SIZE + 1; xx; xx--, matchingPosition++){
				//WORD* TmpB=TmpBImg;
				unsigned short* templateVC;

				//�ϐ�DB_LTrgt_Col, DB_LTrgt_Row, DB_RTrgt_Col, DB_RTrgt_Row���g��
				//�}�b�`���O������e���v���[�g�̐擪�A�h���X��templateVC�ɑ������
				this->targetDB_x = 4;
				this->targetDB_y = 4;
				templateVC = this->templateImageVectorCode[4][4];

				int correlationScore = correlationMinimam;
				vectorCode = matchingPosition;
				for (int y = VCC_TEMPLATE_SIZE / 4; y&&correlationScore > 0; y--, vectorCode += (VCC_INPUT_IMAGE_SIZE_X - 5) - VCC_TEMPLATE_SIZE)
				for (int x = VCC_TEMPLATE_QUARTER_SIZE; x; x--, vectorCode += 4, templateVC++)
					correlationScore += *(correlationMap + (*vectorCode^*templateVC));
				if (correlationScore <= 0)continue;
				correlationMinimam -= correlationScore;
				matchingCoordinate = matchingPosition;
			}
		}
		///*---------------------�T�u�s�N�Z������@------------------------------------------*/
		if (this->subpixelFlag && matchingCoordinate + VCC_INPUT_IMAGE_SIZE_X + 1 < &(inputVectorCodeAllArea[303530-1])){
			int Crrpm1[4] = { VCC_CORRELATION_THRESHOLD, VCC_CORRELATION_THRESHOLD, VCC_CORRELATION_THRESHOLD, VCC_CORRELATION_THRESHOLD };
			for (int i = 0; i < 4; i++)
			{
				//X+1�̑���x
				//WORD* TmpB=TmpBImg;
				unsigned short* templateVC;
				templateVC = this->templateImageVectorCode[this->targetDB_x][this->targetDB_y];

				int correlationScore = VCC_CORRELATION_THRESHOLD;
				switch (i){
				case 0: vectorCode = matchingCoordinate + 1; break;    //X+1�̃A�h���X
				case 1: vectorCode = matchingCoordinate - 1; break;    //X-1�̃A�h���X
				case 2: vectorCode = matchingCoordinate + VCC_INPUT_IMAGE_SIZE_X -5; break;//Y+1�̃A�h���X
				case 3: vectorCode = matchingCoordinate - VCC_INPUT_IMAGE_SIZE_X -5; break;//Y-1�̃A�h���X
				}

				for (int y = VCC_TEMPLATE_SIZE / 4; y&&correlationScore > 0; y--, vectorCode += (VCC_INPUT_IMAGE_SIZE_X - 5) - VCC_TEMPLATE_SIZE)
				for (int x = VCC_TEMPLATE_QUARTER_SIZE; x; x--, vectorCode += 4, templateVC++)
					correlationScore += *(correlationMap + (*vectorCode^*templateVC));
				Crrpm1[i] -= correlationScore;
			}

			subpixelResult_x = (double)(Crrpm1[1] - Crrpm1[0]) / (Crrpm1[0] + Crrpm1[1] - 2 * correlationMinimam) / 2.0;
			subpixelResult_y = (double)(Crrpm1[3] - Crrpm1[2]) / (Crrpm1[2] + Crrpm1[3] - 2 * correlationMinimam) / 2.0;
		}
		/*--------------------�J���}���t�B���^����-------------------------------------*/

		cv::Mat prediction = this->KF->predict();
		cv::Point predictPt(prediction.at<float>(0), prediction.at<float>(1));

		(*measurement)(0) = ((int)matchingCoordinate - (int)inputVectorCodeAllArea) / 2 % (VCC_INPUT_IMAGE_SIZE_X - 5) + VCC_TEMPLATE_SIZE / 2;
		(*measurement)(1) = ((int)matchingCoordinate - (int)inputVectorCodeAllArea) / 2 / (VCC_INPUT_IMAGE_SIZE_X - 5) + VCC_TEMPLATE_SIZE / 2;

		cv::Mat estimated = KF->correct((*measurement));
		kalmanx = estimated.at<float>(0);
		kalmany = estimated.at<float>(1);

		/*---------------------���o���ʂ̌v�Z����---------------------------------------*/
		//�O��̌��o�ʒu
		* matchingParameterPointer = *(matchingParameterPointer + 2);
		*(matchingParameterPointer + 1) = *(matchingParameterPointer + 3);

		if (this->kalmanFlag){
			//����̌��o�ʒu�i���S���W�j�J���}���t�B���^�[����
			*(matchingParameterPointer + 2) = kalmanx;
			*(matchingParameterPointer + 3) = kalmany;
		}
		else{
			//����̌��o�ʒu�i���S���W�j
			*(matchingParameterPointer + 2) = ((int)matchingCoordinate - (int)inputVectorCodeAllArea) / 2 % (VCC_INPUT_IMAGE_SIZE_X - 5) + VCC_TEMPLATE_SIZE / 2;
			*(matchingParameterPointer + 3) = ((int)matchingCoordinate - (int)inputVectorCodeAllArea) / 2 / (VCC_INPUT_IMAGE_SIZE_X - 5) + VCC_TEMPLATE_SIZE / 2;
		}

		//�Ώۂ̗\���ʒu(���S���W�j
		*(matchingParameterPointer + 4) = (std::max)((std::min)(*(matchingParameterPointer + 2), VCC_INPUT_IMAGE_SIZE_X - VCC_TEMPLATE_HALF_X), VCC_TEMPLATE_HALF_X);
		*(matchingParameterPointer + 5) = (std::max)((std::min)(*(matchingParameterPointer + 3), VCC_INPUT_IMAGE_SIZE_Y - VCC_TEMPLATE_HALF_X), VCC_TEMPLATE_HALF_X);

		//�����̈�̗\���ʒu(���S���W)
		*(matchingParameterPointer + 6) = (std::max)((std::min)(*(matchingParameterPointer + 2), VCC_INPUT_IMAGE_SIZE_X - VCC_SEEK_AREA_HALF_SIZE), VCC_SEEK_AREA_HALF_SIZE + 1);
		*(matchingParameterPointer + 7) = (std::max)((std::min)(*(matchingParameterPointer + 3), VCC_INPUT_IMAGE_SIZE_Y - VCC_SEEK_AREA_HALF_SIZE), VCC_SEEK_AREA_HALF_SIZE + 1);

		//���֒l(���S���W)
		*(matchingParameterPointer + 8) = correlationMinimam;
	}
}
