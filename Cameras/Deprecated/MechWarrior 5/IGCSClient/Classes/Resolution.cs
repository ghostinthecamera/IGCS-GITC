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
	/// Simple struct to use for a resolution.
	/// </summary>
	public struct Resolution
	{
		public Resolution(AspectRatio ar, int horizontalResolution, int verticalResolution, string description)
		{
			this.AR = ar;
			this.HorizontalResolution = horizontalResolution;
			this.VerticalResolution = verticalResolution;
			this.Description = description;
		}


		/// <summary>
		/// Creates a new resolution instance with aspect ratio instance from the string specified
		/// </summary>
		/// <param name="resolutionAsString">the resolution specified, which is of the format widthxheight</param>
		/// <returns></returns>
		public static Resolution FromString(string resolutionAsString)
		{
			// format: resolution|aspect ratio. resultion: widthxheight. aspect ratio: widthxheight
			var elements = resolutionAsString.Split('|');
			if(elements.Length != 2)
			{
				return new Resolution(GeneralUtils.CalculateAspectRatio(1920, 1080), 1920, 1080, "Invalid string input");
			}

			var resolutionElements = elements[0].Split('x');
			var horizontalResolution = Convert.ToInt32(resolutionElements[0]);
			var verticalResolution = Convert.ToInt32(resolutionElements[1]);
			var ar = AspectRatio.FromString(elements[1]);
			return new Resolution(ar, horizontalResolution, verticalResolution, string.Format("{0} x {1} ({2})", horizontalResolution, verticalResolution, ar));
		}


		/// <summary>
		/// Produces a string from this instance's values.
		/// Format: resolution|aspect ratio.
		/// resulution: widthxheight.
		/// aspect ratio: width:height
		/// </summary>
		/// <returns></returns>
		public string ToStringForIniFile()
		{
			// Format: resolution|aspect ratio
			// resulution: widthxheight
			// aspect ratio: width:height
			return string.Format("{0}x{1}|{2}", this.HorizontalResolution, this.VerticalResolution, this.AR.ToString(appendDescription: false));
		}


		// these are generated
		public override bool Equals(object obj)
		{
			// no need to compare the aspect ratio too, as it follows from resolution width/height.
			return obj is Resolution resolution &&
				   this.HorizontalResolution == resolution.HorizontalResolution &&
				   this.VerticalResolution == resolution.VerticalResolution;
		}

		// these are generated
		public override int GetHashCode()
		{
			var hashCode = 1445329852;
			hashCode = hashCode * -1521134295 + this.AR.GetHashCode();
			hashCode = hashCode * -1521134295 + this.HorizontalResolution.GetHashCode();
			hashCode = hashCode * -1521134295 + this.VerticalResolution.GetHashCode();
			return hashCode;
		}

		public AspectRatio AR { get;  }
		public int HorizontalResolution { get; }
		public int VerticalResolution { get; }
		public string Description { get; }
	}
}