////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2020, Frans Bouma
// All rights reserved.
// https://github.com/FransBouma/InjectableGenericCameraSystem
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////////////////////////

using System;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Simple struct that can be used to represent an aspect ratio
	/// </summary>
	public struct AspectRatio
	{
		public AspectRatio(int horizontal, int vertical)
		{
			this.Horizontal = horizontal;
			this.Vertical = vertical;
			this.Description = string.Empty;
		}

		/// <summary>
		/// Factor used for calculating the height using a width
		/// </summary>
		public decimal ARFactorHeight
		{
			get { return (decimal)this.Vertical / (decimal)this.Horizontal; }
		}
		
		
		/// <summary>
		/// Factor used for calculating the width using a height
		/// </summary>
		public decimal ARFactorWidth
		{
			get { return (decimal)this.Horizontal / (decimal)this.Vertical; }
		}


		public override string ToString()
		{
			return this.ToString(true);
		}


		public string ToString(bool appendDescription)
		{
			string suffix = string.IsNullOrWhiteSpace(this.Description) || !appendDescription ? string.Empty : string.Format(" ({0})", this.Description);
			return string.Format("{0}:{1}{2}", this.Horizontal, this.Vertical, suffix);
		}


		/// <summary>
		/// Converts the string specified (in the format width:height) to a new aspect ratio instance.
		/// </summary>
		/// <param name="aspectRatioString"></param>
		/// <returns></returns>
		public static AspectRatio FromString(string aspectRatioString)
		{
			// format: width:height
			var elements = aspectRatioString.Split(':');
			if(elements.Length != 2)
			{
				return new AspectRatio(16, 9);
			}
			return new AspectRatio(Convert.ToInt32(elements[0]), Convert.ToInt32(elements[1]));
		}


		// these are generated. Description isn't taken into account.
		public override bool Equals(object obj)
		{
			return obj is AspectRatio ratio &&
				   this.Horizontal == ratio.Horizontal &&
				   this.Vertical == ratio.Vertical;
		}

		// these are generated
		public override int GetHashCode()
		{
			var hashCode = 1211898032;
			hashCode = hashCode * -1521134295 + this.Horizontal.GetHashCode();
			hashCode = hashCode * -1521134295 + this.Vertical.GetHashCode();
			return hashCode;
		}

		public int Horizontal { get; }
		public int Vertical { get; }
		public string Description { get; set; }
	}
}