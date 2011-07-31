///////////////////////////////////////////////////////////////////////////
// Copyright 2011 Evan Mezeske.
//
// This file is part of Digbuild.
// 
// Digbuild is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.
// 
// Digbuild is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Digbuild.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////

#version 130

uniform vec3 zenith_color;
uniform vec3 horizon_color;
uniform float skydome_radius;

varying float height;

void main()
{
    float height_factor = pow( 1.0 - ( height ) / skydome_radius, 1.75 );
    vec3 sky_color = mix( zenith_color, horizon_color, clamp( height_factor, 0.0, 1.0 ) );
    gl_FragColor = vec4( sky_color, 0.0 );
}
