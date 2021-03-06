/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012
 * Computer Graphics Group, University of Siegen, Germany.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 * See http://www.cg.informatik.uni-siegen.de/ for contact information.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#version 120

const int k0 = 7;
const int k1 = 8;
const int k2 = 9;
uniform vec2 step;
uniform float mask0[2 * k0 + 1];
uniform float mask1[2 * k1 + 1];
uniform float mask2[2 * k2 + 1];
uniform float factor0;
uniform float factor1;
uniform float factor2;
uniform sampler2D tex;

void main()
{
    int c;
    float a;
    float g0 = 0.0;
    float g1 = 0.0;
    float g2 = 0.0;

    c = -k2;
    a = texture2D(tex, gl_TexCoord[0].xy + vec2(c, 0) * step).r;
    g2 += mask2[c + k2] * a;
    c = -k1;
    a = texture2D(tex, gl_TexCoord[0].xy + vec2(c, 0) * step).r;
    g1 += mask2[c + k1] * a;
    g2 += mask2[c + k2] * a;
    for (c = -k0; c <= +k0; c++)
    {
        a = texture2D(tex, gl_TexCoord[0].xy + vec2(c, 0) * step).r;
        g0 += mask0[c + k0] * a;
        g1 += mask1[c + k1] * a;
        g2 += mask2[c + k2] * a;
    }
    c = k1;
    a = texture2D(tex, gl_TexCoord[0].xy + vec2(c, 0) * step).r;
    g1 += mask2[c + k1] * a;
    g2 += mask2[c + k2] * a;
    c = k2;
    a = texture2D(tex, gl_TexCoord[0].xy + vec2(c, 0) * step).r;
    g2 += mask2[c + k2] * a;

    g0 *= factor0;
    g1 *= factor1;
    g2 *= factor2;
    gl_FragColor = vec4(texture2D(tex, gl_TexCoord[0].xy).r, g0, g1, g2);
}
