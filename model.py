import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as f
from torch.distributions.uniform import Uniform


class AttentionBlock(nn.Module):
    def __init__(self, N: int):
        super().__init__()
        class ResidualUnit(nn.Module):
            """Simple residual unit."""

            def __init__(self):
                super().__init__()
                self.conv = nn.Sequential(
                    nn.Conv2d(N, N // 2, kernel_size=1, stride=1),
                    nn.ReLU(inplace=True),
                    nn.Conv2d(N // 2, N // 2, kernel_size=3, stride=1, padding=1),
                    nn.ReLU(inplace=True),
                    nn.Conv2d(N // 2, N, kernel_size=1, stride=1),
                )
                self.relu = nn.ReLU(inplace=True)

            def forward(self, x):
                identity = x
                out = self.conv(x)
                out += identity
                out = self.relu(out)
                return out

        self.conv_a = nn.Sequential(ResidualUnit(), ResidualUnit(), ResidualUnit())

        self.conv_b = nn.Sequential(
            ResidualUnit(),
            ResidualUnit(),
            ResidualUnit(),
            nn.Conv2d(N, N, kernel_size=1, stride=1),
        )

    def forward(self, x):
        identity = x
        a = self.conv_a(x)
        b = self.conv_b(x)
        out = a * torch.sigmoid(b)
        out += identity
        return out

class ResidualBottleneck(nn.Module):
    def __init__(self, N=192, act=nn.ReLU) -> None:
        super().__init__()
        self.branch = nn.Sequential(
            nn.Conv2d(N, N // 2, kernel_size=1, stride=1),
            act(),
            nn.Conv2d(N // 2, N // 2, kernel_size=3, stride=1, padding=1),
            act(),
            nn.Conv2d(N // 2, N, kernel_size=1, stride=1)
        )

    def forward(self, x):
        out = x + self.branch(x)

        return out

class Enc(nn.Module):
    def __init__(self, num_features, M):
        #input_features = 3, M = 16
        super(Enc, self).__init__()
        N = 128
        self.g_a = nn.Sequential(
            nn.Conv2d(num_features, N, 5, 2, 2),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            nn.Conv2d(N, N, 5, 2, 2),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            AttentionBlock(N),
            nn.Conv2d(N, N, 5, 2, 2),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            nn.Conv2d(N, M, 3, 1, 1),
            AttentionBlock(M)
        )

    def forward(self, x):
        
        return self.g_a(x)

class Dec(nn.Module):
    def __init__(self, input_features, M):
        super(Dec, self).__init__()
        N = 128
        self.g_s = nn.Sequential(
            AttentionBlock(M),
            nn.Conv2d(M, N, 3, 1, 1),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            nn.ConvTranspose2d(N, N, 5, 2, 2, 1),
            AttentionBlock(N),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            nn.ConvTranspose2d(N, N, 5, 2, 2, 1),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            ResidualBottleneck(N),
            nn.ConvTranspose2d(N, input_features, 5, 2, 2, 1)
        )

    def forward(self, x):

        return self.g_s(x)

class Image_coding(nn.Module):
    def __init__(self, input_features, M):
        #input_features = 3, M = 16
        super(Image_coding, self).__init__()
        self.encoder = Enc(input_features, M)
        self.decoder = Dec(input_features, M)
    def add_noise(self, x):
        noise = np.random.uniform(-0.5, 0.5, x.size())
        noise = torch.Tensor(noise).cuda()
        return x + noise
    def forward(self, x, if_training):
        x1 = self.encoder(x)
        if if_training == 0:
            xq1 = self.add_noise(x1)
        elif if_training == 1:
            xq1 = UniverseQuant.apply(x1)
        else:
            xq1 = torch.round(x1)
        output = self.decoder(xq1)
        return [output, x1, xq1]

class UniverseQuant(torch.autograd.Function):
    @staticmethod
    def forward(ctx, x):
        #b = np.random.uniform(-1, 1)
        #uniform_distribution = Uniform(-0.5*torch.ones(x.size()), 0.5*torch.ones(x.size())).sample().cuda()
        #return x+uniform_distribution#)-uniform_distribution
        #b = np.random.uniform(-1, 1)
        #uniform_distribution = Uniform(-0.5*torch.ones(x.size())
        #                               * (2**b), 0.5*torch.ones(x.size())*(2**b)).sample().cuda()
        #return torch.round(x+uniform_distribution)-uniform_distribution
        return torch.round(x)

    @staticmethod
    def backward(ctx, g):

        return g
