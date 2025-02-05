import argparse
import os
import numpy as np
import torch
import torch.nn as nn
from PIL import Image
from torch.autograd import Variable
from torch.utils.data import DataLoader, Dataset
from torchvision import transforms
from torchvision.datasets import ImageFolder
import model

def encode(args):
    image_comp = model.Image_coding(3, args.channels)
    image_comp.load_state_dict(torch.load(args.model_name, map_location='cpu'))
    image_comp.cuda()
    
    img = Image.open(args.image_name)
    img = np.array(img)/255.0
    H, W, _ = img.shape
    C = 3
    H_pad = int(np.ceil(H/32)*32)
    W_pad = int(np.ceil(W/32)*32)
    img2 = np.zeros((H_pad,W_pad,3))
    img2[:H,:W,:] = img
    img2 = img2.astype('float32')
    img2 = torch.FloatTensor(img2)
    img2 = img2.permute(2, 0, 1).contiguous()
    img2 = img2.view(1, C, H_pad, W_pad)
    img2 = img2.cuda()

    x1 = image_comp.encoder(img2)
    xq1 = torch.round(x1)

    xq2 = xq1.cpu().detach().numpy()[0] + 128
    if xq2.min() < 0 or xq2.max() > 255:
        print("out range")
    xq2 = xq2.astype('uint8')
    feature_name = args.image_name[:-4]
    xq2.tofile(feature_name+'.yuv')

    os.system("encoder.exe -i "+feature_name+".yuv -o "+args.bin_name+" -r "+str(H_pad//8)+" -c "+str(W_pad//8)+" -f "+str(args.channels)+" >"+feature_name+"enc.txt")     
    
def decode(args):
    image_comp = model.Image_coding(3, args.channels)
    image_comp.load_state_dict(torch.load(args.model_name, map_location='cpu'))
    image_comp.cuda()
    
    os.system("decoder.exe -i "+args.bin_name+" -o "+args.bin_name[:-4]+"dec.yuv >"+args.bin_name[:-4]+"dec.txt")
    
    fp = open(args.bin_name[:-4]+"dec.txt",'r')
    height = int(fp.readline().split(':')[1])
    width = int(fp.readline().split(':')[1])
    fp.close()
    
    fp=open(args.bin_name[:-4]+"dec.yuv",'rb')    #dec
    yuv = np.zeros((height*width*args.channels,1), dtype=np.uint8)
    yuv = np.fromfile(fp, dtype=np.uint8, count=height*width*args.channels, sep='')
    fp.close()
    yuv = yuv.reshape((args.channels, height, width))
    yuv = yuv.astype('float32')
    yuv = yuv - 128
    yuv = torch.FloatTensor(yuv)
    yuv = yuv.contiguous()
    yuv = yuv.view(1, args.channels, height, width)
    yuv = yuv.cuda()

    fake = image_comp.decoder(yuv)

    fake = torch.clamp(fake, min=0., max=1.0)
    fake = torch.round(fake*255.0)
    out = fake.data[0].cpu().numpy()
    out_img = out.transpose(1, 2, 0)

    out_img = np.ascontiguousarray(out_img.astype('uint8'))
    img = Image.fromarray(out_img)
    img.save(args.rec_image_name)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--channels", type=int, default=64, help="(mse1 mse4 mse8 mse16 msssim1 msssim4 msssim8 mssism16 64channels) (mse32 mse64 msssim32 msssim64 128channels)")
    parser.add_argument('--image_name', type=str)
    parser.add_argument('--model_name', type=str, default='models/mse1.pth')
    parser.add_argument('--bin_name', type=str)
    parser.add_argument('--rec_image_name', type=str)
    args = parser.parse_args()
    encode(args)
    decode(args)